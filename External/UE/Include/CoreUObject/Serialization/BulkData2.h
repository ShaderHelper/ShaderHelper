// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BulkDataCommon.h"
#include "BulkDataBuffer.h"
#include "Containers/StringView.h"
#include "Async/AsyncFileHandle.h"
#include "IO/IoDispatcher.h"
#include "Misc/PackagePath.h"
#include "Misc/PackageSegment.h"

struct FOwnedBulkDataPtr;
class FBulkDataBase;
class FLinkerLoad;
class IMappedFileHandle;
class IMappedFileRegion;

/** A loose hash value that can be created from either a filenames or a FIoChunkId */
using FIoFilenameHash = uint32;
const FIoFilenameHash INVALID_IO_FILENAME_HASH = 0;
/** Helpers to create the hash from a filename. Returns IOFILENAMEHASH_NONE if and only if the filename is empty. */
COREUOBJECT_API FIoFilenameHash MakeIoFilenameHash(const FString& Filename);
/** Helpers to create the hash from a FPackagePath. Returns IOFILENAMEHASH_NONE if and only if the PackagePath is empty. */
COREUOBJECT_API FIoFilenameHash MakeIoFilenameHash(const FPackagePath& Filename);
/** Helpers to create the hash from a chunk id. Returns IOFILENAMEHASH_NONE if and only if the chunk id is invalid. */
COREUOBJECT_API FIoFilenameHash MakeIoFilenameHash(const FIoChunkId& ChunkID);

/**
 * Represents an IO request from the BulkData streaming API.
 *
 * It functions pretty much the same as IAsyncReadRequest expect that it also holds
 * the file handle as well.
 */
class COREUOBJECT_API IBulkDataIORequest
{
public:
	virtual ~IBulkDataIORequest() {}

	virtual bool PollCompletion() const = 0;
	virtual bool WaitCompletion(float TimeLimitSeconds = 0.0f) = 0;

	virtual uint8* GetReadResults() = 0;
	virtual int64 GetSize() const = 0;

	virtual void Cancel() = 0;
};

namespace UE::BulkData::Private
{

IAsyncReadFileHandle* CreateAsyncReadHandle(const FIoChunkId& InChunkID);

} // namespace UE::BulkData::Private


struct FBulkDataOrId
{
	using FileToken = uint64;

	union
	{
		FileToken Token;
		uint64 PackageID;
	}; // Note that the union will end up being 16 bytes with padding
};
DECLARE_INTRINSIC_TYPE_LAYOUT(FBulkDataOrId);

// Declare this here rather than BulkDataCommon.h
DECLARE_INTRINSIC_TYPE_LAYOUT(EBulkDataFlags);

/**
 * This is a wrapper for the BulkData memory allocation so we can use a single pointer to either
 * reference a straight memory allocation or in the case that the BulkData object represents a 
 * memory mapped file region, a FOwnedBulkDataPtr.
 * This makes the code more complex but it means that we do not pay any additional memory cost when
 * memory mapping isn't being used at a small cpu cost. However the number of BulkData object usually
 * means that the memory saving is worth it compared to how infrequently the memory accessors are 
 * actually called.
 *
 * Note: We use a flag set in the owning BulkData object to tell us what storage type we are using 
 * so all accessors require that a pointer to the parent object be passed in.
 */
class FBulkDataAllocation
{
public:
	FBulkDataAllocation() = default;
	~FBulkDataAllocation() = default;

	bool IsLoaded() const
	{
		return Allocation.RawData != nullptr; // Doesn't matter which allocation we test
	}
	void Free(FBulkDataBase* Owner);

	// Set as a raw buffer
	void* AllocateData(FBulkDataBase* Owner, SIZE_T SizeInBytes); 
	void* ReallocateData(FBulkDataBase* Owner, SIZE_T SizeInBytes);
	void SetData(FBulkDataBase* Owner, void* Buffer);

	// Set as memory mapped
	void SetMemoryMappedData(FBulkDataBase* Owner, IMappedFileHandle* MappedHandle, IMappedFileRegion* MappedRegion);

	// Getters		
	void* GetAllocationForWrite(const FBulkDataBase* Owner) const;
	const void* GetAllocationReadOnly(const FBulkDataBase* Owner) const;

	FOwnedBulkDataPtr* StealFileMapping(FBulkDataBase* Owner);
	void Swap(FBulkDataBase* Owner, void** DstBuffer);

private:

	union FAllocation
	{
		/** Raw memory allocations, allocated via FMemory::Malloc/Realloc */
		void* RawData;
		/** Wrapper around memory mapped allocations allocated via new */
		FOwnedBulkDataPtr* MemoryMappedData;
	};

	FAllocation Allocation{ nullptr };
};
DECLARE_INTRINSIC_TYPE_LAYOUT(FBulkDataAllocation);

/**
 * Callback to use when making streaming requests
 */
typedef TFunction<void(bool bWasCancelled, IBulkDataIORequest*)> FBulkDataIORequestCallBack;

/**
 * Creates a bulk data request from the I/O store backend.
 */
TUniquePtr<IBulkDataIORequest> CreateBulkDataIoDispatcherRequest(
	const FIoChunkId& InChunkID,
	int64 InOffsetInBulkData = 0,
	int64 InBytesToRead = INDEX_NONE,
	FBulkDataIORequestCallBack* InCompleteCallback = nullptr,
	uint8* InUserSuppliedMemory = nullptr,
	int32 InPriority = IoDispatcherPriority_Low);

/**
 * @documentation @todo documentation
 */
class COREUOBJECT_API FBulkDataBase
{
	DECLARE_TYPE_LAYOUT(FBulkDataBase, NonVirtual);

public:
	using BulkDataRangeArray = TArray<FBulkDataBase*, TInlineAllocator<8>>;

	static void				SetIoDispatcher(FIoDispatcher* InIoDispatcher) { IoDispatcher = InIoDispatcher; }
	static FIoDispatcher*	GetIoDispatcher() { return IoDispatcher; }
public:
	static constexpr FBulkDataOrId::FileToken InvalidToken = FBulkDataOrId::FileToken(INDEX_NONE);

	FBulkDataBase()
	{
		Data.Token = InvalidToken;
	}

	FBulkDataBase(const FBulkDataBase& Other)
	{
		// Need some partial initialization of operator= will try to release the token!
		Data.Token = InvalidToken;

		*this = Other;
	}

	FBulkDataBase(FBulkDataBase&& Other);
	FBulkDataBase& operator=(const FBulkDataBase& Other);

	~FBulkDataBase();
protected:

	void Serialize(FArchive& Ar, UObject* Owner, int32 Index, bool bAttemptFileMapping, int32 ElementSize);

public:
	void* Lock(uint32 LockFlags);
	const void* LockReadOnly() const;
	void Unlock() const;
	bool IsLocked() const;

	void* Realloc(int64 SizeInBytes);

	/**
	 * Retrieves a copy of the bulk data.
	 *
	 * @param Dest [in/out] Pointer to pointer going to hold copy, can point to nullptr in which case memory is allocated
	 * @param bDiscardInternalCopy Whether to discard/ free the potentially internally allocated copy of the data
	 */
	void GetCopy(void** Dest, bool bDiscardInternalCopy);

	int64 GetBulkDataSize() const;

	void SetBulkDataFlags(uint32 BulkDataFlagsToSet);
	void ResetBulkDataFlags(uint32 BulkDataFlagsToSet);
	void ClearBulkDataFlags(uint32 BulkDataFlagsToClear);
	uint32 GetBulkDataFlags() const { return BulkDataFlags; }

	bool CanLoadFromDisk() const;

	/**
	 * Returns true if the data references a file that currently exists and can be referenced by the file system.
	 */
	bool DoesExist() const;

	bool IsStoredCompressedOnDisk() const;
	FName GetDecompressionFormat() const;

	bool IsBulkDataLoaded() const { return DataAllocation.IsLoaded(); }

	// TODO: The flag tests could be inline if we fixed the header dependency issues (the flags are defined in Bulkdata.h at the moment)
	bool IsAvailableForUse() const;
	bool IsDuplicateNonOptional() const;
	bool IsOptional() const;
	bool IsInlined() const;
	UE_DEPRECATED(4.25, "Use ::IsInSeparateFile() instead")
	FORCEINLINE bool InSeperateFile() const { return IsInSeparateFile(); }
	bool IsInSeparateFile() const;
	bool IsSingleUse() const;
	UE_DEPRECATED(4.26, "Use ::IsFileMemoryMapped() instead")
	bool IsMemoryMapped() const { return IsFileMemoryMapped(); }
	bool IsFileMemoryMapped() const;
	bool IsDataMemoryMapped() const;
	bool IsUsingIODispatcher() const;

	IAsyncReadFileHandle* OpenAsyncReadHandle() const;

	IBulkDataIORequest* CreateStreamingRequest(EAsyncIOPriorityAndFlags Priority, FBulkDataIORequestCallBack* CompleteCallback, uint8* UserSuppliedMemory) const;
	IBulkDataIORequest* CreateStreamingRequest(int64 OffsetInBulkData, int64 BytesToRead, EAsyncIOPriorityAndFlags Priority, FBulkDataIORequestCallBack* CompleteCallback, uint8* UserSuppliedMemory) const;

	static IBulkDataIORequest* CreateStreamingRequestForRange(const BulkDataRangeArray& RangeArray, EAsyncIOPriorityAndFlags Priority, FBulkDataIORequestCallBack* CompleteCallback);

	/**
	 * Clears/removes any currently allocated data payload.
	 *
	 * Note that once this has been called, the bulkdata object will no longer be able to reload
	 * it's payload from disk!
	 */
	void RemoveBulkData();

	/**
	* Initiates a new asynchronous operation to load the bulkdata from disk assuming that it is not already
	* loaded.
	* Note that a new asynchronous loading operation will not be created if one is already in progress.
	*
	* @return True if an asynchronous loading operation is in progress by the time that the method returns
	* and false if the data is already loaded or cannot be loaded from disk.
	*/
	bool StartAsyncLoading();
	bool IsAsyncLoadingComplete() const;

	// Added for compatibility with the older BulkData system
	int64 GetBulkDataOffsetInFile() const;

	UE_DEPRECATED(5.0, "Use GetPackagePath instead")
	FString GetFilename() const;

	/** Returns the PackagePath this bulkdata resides in */
	FPackagePath GetPackagePath() const;

	/** Returns which segment of its PackagePath this bulkdata resides in */
	EPackageSegment GetPackageSegment() const;

	/** 
	 * Returns the io filename hash associated with this bulk data.
	 *
	 * @return Hash or INVALID_IO_FILENAME_HASH if invalid.
	 **/
	FIoFilenameHash GetIoFilenameHash() const;


public:
	// The following methods are for compatibility with SoundWave.cpp which assumes memory mapping.
	void ForceBulkDataResident(); // Is closer to MakeSureBulkDataIsLoaded in the old system but kept the name due to existing use
	FOwnedBulkDataPtr* StealFileMapping();

	FIoChunkId CreateChunkId() const;
private:
	friend FBulkDataAllocation;

	void SetRuntimeBulkDataFlags(uint32 BulkDataFlagsToSet);
	void ClearRuntimeBulkDataFlags(uint32 BulkDataFlagsToClear);

	/** Returns if the offset needs fixing when serialized */
	bool NeedsOffsetFixup() const;

	/**
	 * Poll to see if it is safe to discard the data owned by the Bulkdata object
	 *
	 * @return True if we are allowed to discard the existing data in the Bulkdata object.
	 */
	bool CanDiscardInternalData() const;

	void ProcessDuplicateData(EBulkDataFlags NewFlags, int64 NewSizeOnDisk, int64 NewOffset, const UPackage* Package,
		const FPackagePath* PackagePath, const FLinkerLoad* Linker);
	void ConditionalSetInlineAlwaysAllowDiscard(bool bPackageUsesIoStore);
	void SerializeDuplicateData(FArchive& Ar, EBulkDataFlags& OutBulkDataFlags, int64& OutBulkDataSizeOnDisk, int64& OutBulkDataOffsetInFile);
	void SerializeBulkData(FArchive& Ar, void* DstBuffer, int64 DataLength);

	bool MemoryMapBulkData(const FPackagePath& PackagePath, EPackageSegment PackageSegment, int64 OffsetInBulkData, int64 BytesToRead);

	// Methods for dealing with the allocated data
	FORCEINLINE void* AllocateData(SIZE_T SizeInBytes) { return DataAllocation.AllocateData(this, SizeInBytes); }
	FORCEINLINE void* ReallocateData(SIZE_T SizeInBytes) { return DataAllocation.ReallocateData(this, SizeInBytes); }
	FORCEINLINE void  FreeData() { DataAllocation.Free(this); }
	FORCEINLINE void* GetDataBufferForWrite() const { return DataAllocation.GetAllocationForWrite(this); }
	FORCEINLINE const void* GetDataBufferReadOnly() const { return DataAllocation.GetAllocationReadOnly(this); }

	/** Blocking call that waits until any pending async load finishes */
	void FlushAsyncLoading();
	
	EPackageSegment GetPackageSegmentFromFlags() const;

private:
	using AsyncCallback = TFunction<void(TIoStatusOr<FIoBuffer>)>;

	void LoadDataDirectly(void** DstBuffer);
	void LoadDataAsynchronously(AsyncCallback&& Callback);

	// Used by LoadDataDirectly/LoadDataAsynchronously
	void InternalLoadFromPackageResource(void** DstBuffer);
	void InternalLoadFromIoStore(void** DstBuffer);
	void InternalLoadFromIoStoreAsync(void** DstBuffer, AsyncCallback&& Callback);

	static FIoDispatcher* IoDispatcher;

	LAYOUT_FIELD(FBulkDataOrId, Data);
	LAYOUT_FIELD(FBulkDataAllocation, DataAllocation);
	LAYOUT_FIELD_INITIALIZED(int64, BulkDataSize, 0);
	LAYOUT_FIELD_INITIALIZED(int64, BulkDataOffset, INDEX_NONE);
	LAYOUT_FIELD_INITIALIZED(EBulkDataFlags, BulkDataFlags, EBulkDataFlags::BULKDATA_None);
	LAYOUT_MUTABLE_FIELD_INITIALIZED(uint8, LockStatus, 0); // Mutable so that the read only lock can be const
};

/**
 * @documentation @todo documentation
 */
template<typename ElementType>
class COREUOBJECT_API FUntypedBulkData2 : public FBulkDataBase
{
	// In the older Bulkdata system the data was being loaded as if it was POD with the option to opt out
	// but nothing actually opted out. This check should help catch if any non-POD data was actually being
	// used or not.
	static_assert(TIsPODType<ElementType>::Value, "FUntypedBulkData2 is limited to POD types!");
public:
	FORCEINLINE FUntypedBulkData2() {}

	void Serialize(FArchive& Ar, UObject* Owner, int32 Index, bool bAttemptFileMapping)
	{
		FBulkDataBase::Serialize(Ar, Owner, Index, bAttemptFileMapping, sizeof(ElementType));
	}
	
	// @TODO: The following two ::Serialize methods are a work around for the default parameters in the old 
	// BulkData api that are not used anywhere and to avoid causing code compilation issues for licensee code.
	// At some point in the future we should remove Index and bAttemptFileMapping from both the old and new 
	// BulkData api implementations of ::Serialize and then use UE_DEPRECATE to update existing code properly.
	FORCEINLINE void Serialize(FArchive& Ar, UObject* Owner)
	{	
		Serialize(Ar, Owner, INDEX_NONE, false);
	}

	// @TODO: See above
	FORCEINLINE void Serialize(FArchive& Ar, UObject* Owner, int32 Index)
	{
		Serialize(Ar, Owner, Index, false);
	}

	/**
	 * Returns the number of elements held by the BulkData object.
	 *
	 * @return Number of elements.
	 */
	int64 GetElementCount() const 
	{ 
		return GetBulkDataSize() / GetElementSize(); 
	}

	/**
	 * Returns size in bytes of single element.
	 *
	 * @return The size of the element.
	 */
	int32 GetElementSize() const
	{ 
		return sizeof(ElementType); 
	}

	ElementType* Lock(uint32 LockFlags)
	{
		return (ElementType*)FBulkDataBase::Lock(LockFlags);
	}

	const ElementType* LockReadOnly() const
	{
		return (const ElementType*)FBulkDataBase::LockReadOnly();
	}

	ElementType* Realloc(int64 InElementCount)
	{
		return (ElementType*)FBulkDataBase::Realloc(InElementCount * sizeof(ElementType));
	}

	/**
	 * Returns a copy encapsulated by a FBulkDataBuffer.
	 *
	 * @param RequestedElementCount If set to greater than 0, the returned FBulkDataBuffer will be limited to
	 * this number of elements. This will give an error if larger than the actual number of elements in the BulkData object.
	 * @param bDiscardInternalCopy If true then the BulkData object will free it's internal buffer once called.
	 *
	 * @return A FBulkDataBuffer that owns a copy of the BulkData, this might be a subset of the data depending on the value of RequestedSize.
	 */
	FORCEINLINE FBulkDataBuffer<ElementType> GetCopyAsBuffer(int64 RequestedElementCount, bool bDiscardInternalCopy)
	{
		const int64 MaxElementCount = GetElementCount();

		check(RequestedElementCount <= MaxElementCount);

		ElementType* Ptr = nullptr;
		GetCopy((void**)& Ptr, bDiscardInternalCopy);

		const int64 BufferSize = (RequestedElementCount > 0 ? RequestedElementCount : MaxElementCount);

		return FBulkDataBuffer<ElementType>(Ptr, BufferSize);
	}
};

// Commonly used types
using FByteBulkData2 = FUntypedBulkData2<uint8>;
using FWordBulkData2 = FUntypedBulkData2<uint16>;
using FIntBulkData2 = FUntypedBulkData2<int32>;
using FFloatBulkData2 = FUntypedBulkData2<float>;
