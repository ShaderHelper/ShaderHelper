// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

#if PLATFORM_64BITS && PLATFORM_HAS_FPlatformVirtualMemoryBlock
#include "Misc/AssertionMacros.h"
#include "Misc/ScopeLock.h"
#include "HAL/MemoryBase.h"
#include "HAL/UnrealMemory.h"
#include "Math/NumericLimits.h"
#include "Templates/AlignmentTemplates.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformTLS.h"
#include "HAL/Allocators/CachedOSPageAllocator.h"
#include "HAL/Allocators/PooledVirtualMemoryAllocator.h"
#include "HAL/PlatformMath.h"
#include "HAL/LowLevelMemTracker.h"
#include "HAL/MallocBinnedCommon.h"
#include "Misc/ScopeLock.h"


#define BINNEDARENA_MAX_GMallocBinnedArenaMaxBundlesBeforeRecycle (8)

#define COLLECT_BINNEDARENA_STATS (!UE_BUILD_SHIPPING)

#if COLLECT_BINNEDARENA_STATS
#define MBA_STAT(x) x
#else
#define MBA_STAT(x)
#endif

PRAGMA_DISABLE_UNSAFE_TYPECAST_WARNINGS

class CORE_API FMallocBinnedArena final : public FMalloc
{
	struct Private;

	struct FPoolInfoSmall;
	struct FPoolInfoLarge;
	struct PoolHashBucket;
	struct FPoolTable;
	struct FGlobalRecycler;


	struct FFreeBlock
	{
		enum
		{
			CANARY_VALUE = 0xc3
		};

		FORCEINLINE FFreeBlock(uint32 InPageSize, uint32 InBlockSize, uint32 InPoolIndex, uint8 MinimumAlignmentShift)
			: BlockSizeShifted(InBlockSize >> MinimumAlignmentShift)
			, PoolIndex(InPoolIndex)
			, Canary(CANARY_VALUE)
			, NextFreeIndex(MAX_uint32)
		{
			check(InPoolIndex < MAX_uint8 && (InBlockSize >> MinimumAlignmentShift) <= MAX_uint16);
			NumFreeBlocks = InPageSize / InBlockSize;
		}

		FORCEINLINE uint32 GetNumFreeRegularBlocks() const
		{
			return NumFreeBlocks;
		}
		FORCEINLINE bool IsCanaryOk() const
		{
			return Canary == FFreeBlock::CANARY_VALUE;
		}

		FORCEINLINE void CanaryTest() const
		{
			if (!IsCanaryOk())
			{
				CanaryFail();
			}
		}
		void CanaryFail() const;

		FORCEINLINE void* AllocateRegularBlock(uint8 MinimumAlignmentShift)
		{
			--NumFreeBlocks;
			return (uint8*)this + NumFreeBlocks * (uint32(BlockSizeShifted) << MinimumAlignmentShift);
		}

		uint16 BlockSizeShifted;		// Size of the blocks that this list points to >> ArenaParams.MinimumAlignmentShift
		uint8 PoolIndex;				// Index of this pool
		uint8 Canary;					// Constant value of 0xe3
		uint32 NumFreeBlocks;          // Number of consecutive free blocks here, at least 1.
		uint32 NextFreeIndex;          // Next free block or MAX_uint32
	};

	struct FPoolTable
	{
		uint32 BlockSize;
		uint16 BlocksPerBlockOfBlocks;
		uint8 PagesPlatformForBlockOfBlocks;

		FBitTree BlockOfBlockAllocationBits; // one bits in here mean the virtual memory is committed
		FBitTree BlockOfBlockIsExhausted;    // one bit in here means the pool is completely full

		uint32 NumEverUsedBlockOfBlocks;
		FPoolInfoSmall** PoolInfos;

		uint64 UnusedAreaOffsetLow;
	};

	struct FPtrToPoolMapping
	{
		FPtrToPoolMapping()
			: PtrToPoolPageBitShift(0)
			, HashKeyShift(0)
			, PoolMask(0)
			, MaxHashBuckets(0)
		{
		}
		explicit FPtrToPoolMapping(uint32 InPageSize, uint64 InNumPoolsPerPage, uint64 AddressLimit)
		{
			Init(InPageSize, InNumPoolsPerPage, AddressLimit);
		}

		void Init(uint32 InPageSize, uint64 InNumPoolsPerPage, uint64 AddressLimit)
		{
			uint64 PoolPageToPoolBitShift = FPlatformMath::CeilLogTwo(InNumPoolsPerPage);

			PtrToPoolPageBitShift = FPlatformMath::CeilLogTwo(InPageSize);
			HashKeyShift = PtrToPoolPageBitShift + PoolPageToPoolBitShift;
			PoolMask = (1ull << PoolPageToPoolBitShift) - 1;
			MaxHashBuckets = AddressLimit >> HashKeyShift;
		}

		FORCEINLINE void GetHashBucketAndPoolIndices(const void* InPtr, uint32& OutBucketIndex, UPTRINT& OutBucketCollision, uint32& OutPoolIndex) const
		{
			OutBucketCollision = (UPTRINT)InPtr >> HashKeyShift;
			OutBucketIndex = uint32(OutBucketCollision & (MaxHashBuckets - 1));
			OutPoolIndex = ((UPTRINT)InPtr >> PtrToPoolPageBitShift) & PoolMask;
		}

		FORCEINLINE uint64 GetMaxHashBuckets() const
		{
			return MaxHashBuckets;
		}

	private:
		/** Shift to apply to a pointer to get the reference from the indirect tables */
		uint64 PtrToPoolPageBitShift;
		/** Shift required to get required hash table key. */
		uint64 HashKeyShift;
		/** Used to mask off the bits that have been used to lookup the indirect table */
		uint64 PoolMask;
		// PageSize dependent constants
		uint64 MaxHashBuckets;
	};

	struct FBundleNode
	{
		FBundleNode* NextNodeInCurrentBundle;
		union
		{
			FBundleNode* NextBundle;
			int32 Count;
		};
	};

	struct FBundle
	{
		FORCEINLINE FBundle()
		{
			Reset();
		}

		FORCEINLINE void Reset()
		{
			Head = nullptr;
			Count = 0;
		}

		FORCEINLINE void PushHead(FBundleNode* Node)
		{
			Node->NextNodeInCurrentBundle = Head;
			Node->NextBundle = nullptr;
			Head = Node;
			Count++;
		}

		FORCEINLINE FBundleNode* PopHead()
		{
			FBundleNode* Result = Head;

			Count--;
			Head = Head->NextNodeInCurrentBundle;
			return Result;
		}

		FBundleNode* Head;
		uint32       Count;
	};

	struct FFreeBlockList
	{
		// return true if we actually pushed it
		FORCEINLINE bool PushToFront(void* InPtr, uint32 InPoolIndex, uint32 InBlockSize, const FArenaParams& LocalArenaParams)
		{
			checkSlow(InPtr);

			if ((PartialBundle.Count >= (uint32)LocalArenaParams.MaxBlocksPerBundle) | (PartialBundle.Count * InBlockSize >= (uint32)LocalArenaParams.MaxSizePerBundle))
			{
				if (FullBundle.Head)
				{
					return false;
				}
				FullBundle = PartialBundle;
				PartialBundle.Reset();
			}
			PartialBundle.PushHead((FBundleNode*)InPtr);
			return true;
		}
		FORCEINLINE bool CanPushToFront(uint32 InPoolIndex, uint32 InBlockSize, const FArenaParams& LocalArenaParams)
		{
			return !((!!FullBundle.Head) & ((PartialBundle.Count >= (uint32)LocalArenaParams.MaxBlocksPerBundle) | (PartialBundle.Count * InBlockSize >= (uint32)LocalArenaParams.MaxSizePerBundle)));
		}
		FORCEINLINE void* PopFromFront(uint32 InPoolIndex)
		{
			if ((!PartialBundle.Head) & (!!FullBundle.Head))
			{
				PartialBundle = FullBundle;
				FullBundle.Reset();
			}
			return PartialBundle.Head ? PartialBundle.PopHead() : nullptr;
		}

		// tries to recycle the full bundle, if that fails, it is returned for freeing
		FBundleNode* RecyleFull(FArenaParams& LocalArenaParams, FGlobalRecycler& GGlobalRecycler, uint32 InPoolIndex);
		bool ObtainPartial(FArenaParams& LocalArenaParams, FGlobalRecycler& GGlobalRecycler, uint32 InPoolIndex);
		FBundleNode* PopBundles(uint32 InPoolIndex);
	private:
		FBundle PartialBundle;
		FBundle FullBundle;
	};

	struct FPerThreadFreeBlockLists
	{
		FORCEINLINE static FPerThreadFreeBlockLists* Get(uint32 BinnedArenaTlsSlot)
		{
			return BinnedArenaTlsSlot ? (FPerThreadFreeBlockLists*)FPlatformTLS::GetTlsValue(BinnedArenaTlsSlot) : nullptr;
		}
		static void SetTLS(FMallocBinnedArena& Allocator);
		static int64 ClearTLS(FMallocBinnedArena& Allocator);

		FPerThreadFreeBlockLists(uint32 PoolCount)
			: AllocatedMemory(0)
		{ 
			FreeLists.AddDefaulted(PoolCount);
		}

		FORCEINLINE void* Malloc(uint32 InPoolIndex)
		{
			return FreeLists[InPoolIndex].PopFromFront(InPoolIndex);
		}
		// return true if the pointer was pushed
		FORCEINLINE bool Free(void* InPtr, uint32 InPoolIndex, uint32 InBlockSize, const FArenaParams& LocalArenaParams)
		{
			return FreeLists[InPoolIndex].PushToFront(InPtr, InPoolIndex, InBlockSize, LocalArenaParams);
		}
		// return true if a pointer can be pushed
		FORCEINLINE bool CanFree(uint32 InPoolIndex, uint32 InBlockSize, const FArenaParams& LocalArenaParams)
		{
			return FreeLists[InPoolIndex].CanPushToFront(InPoolIndex, InBlockSize, LocalArenaParams);
		}
		// returns a bundle that needs to be freed if it can't be recycled
		FBundleNode* RecycleFullBundle(FArenaParams& LocalArenaParams, FGlobalRecycler& GlobalRecycler, uint32 InPoolIndex)
		{
			return FreeLists[InPoolIndex].RecyleFull(LocalArenaParams, GlobalRecycler, InPoolIndex);
		}
		// returns true if we have anything to pop
		bool ObtainRecycledPartial(FArenaParams& LocalArenaParams, FGlobalRecycler& GlobalRecycler, uint32 InPoolIndex)
		{
			return FreeLists[InPoolIndex].ObtainPartial(LocalArenaParams, GlobalRecycler, InPoolIndex);
		}
		FBundleNode* PopBundles(uint32 InPoolIndex)
		{
			return FreeLists[InPoolIndex].PopBundles(InPoolIndex);
		}
		int64 AllocatedMemory;
		TArray<FFreeBlockList> FreeLists;
	};

	struct FGlobalRecycler
	{
		void Init(uint32 PoolCount)
		{
			Bundles.AddDefaulted(PoolCount);
		}
		bool PushBundle(uint32 NumCachedBundles, uint32 InPoolIndex, FBundleNode* InBundle)
		{
			for (uint32 Slot = 0; Slot < NumCachedBundles && Slot < BINNEDARENA_MAX_GMallocBinnedArenaMaxBundlesBeforeRecycle; Slot++)
			{
				if (!Bundles[InPoolIndex].FreeBundles[Slot])
				{
					if (!FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Bundles[InPoolIndex].FreeBundles[Slot], InBundle, nullptr))
					{
						return true;
					}
				}
			}
			return false;
		}

		FBundleNode* PopBundle(uint32 NumCachedBundles, uint32 InPoolIndex)
		{
			for (uint32 Slot = 0; Slot < NumCachedBundles && Slot < BINNEDARENA_MAX_GMallocBinnedArenaMaxBundlesBeforeRecycle; Slot++)
			{
				FBundleNode* Result = Bundles[InPoolIndex].FreeBundles[Slot];
				if (Result)
				{
					if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Bundles[InPoolIndex].FreeBundles[Slot], nullptr, Result) == Result)
					{
						return Result;
					}
				}
			}
			return nullptr;
		}

	private:
		struct FPaddedBundlePointer
		{
			FBundleNode* FreeBundles[BINNEDARENA_MAX_GMallocBinnedArenaMaxBundlesBeforeRecycle];
			FPaddedBundlePointer()
			{
				DefaultConstructItems<FBundleNode*>(FreeBundles, BINNEDARENA_MAX_GMallocBinnedArenaMaxBundlesBeforeRecycle);
			}
		};
		TArray<FPaddedBundlePointer> Bundles;
	};


	FORCEINLINE uint64 PoolIndexFromPtr(const void* Ptr) 
	{
		if (PoolSearchDiv == 0)
		{
			return (UPTRINT(Ptr) - UPTRINT(PoolBaseVMPtr[0])) >> ArenaParams.MaxMemoryPerBlockSizeShift;
		}
		uint64 PoolIndex = ArenaParams.PoolCount;
		if (((uint8*)Ptr >= PoolBaseVMPtr[0]) & ((uint8*)Ptr < HighestPoolBaseVMPtr + ArenaParams.MaxMemoryPerBlockSize))
		{
			PoolIndex = uint64((uint8*)Ptr - PoolBaseVMPtr[0]) / PoolSearchDiv;
			if (PoolIndex >= ArenaParams.PoolCount)
			{
				PoolIndex = ArenaParams.PoolCount - 1;
			}
			if ((uint8*)Ptr < PoolBaseVMPtr[(int32)PoolIndex])
			{
				do
				{
					PoolIndex--;
					check(PoolIndex < ArenaParams.PoolCount);
				} while ((uint8*)Ptr < PoolBaseVMPtr[(int32)PoolIndex]);
				if ((uint8*)Ptr >= PoolBaseVMPtr[(int32)PoolIndex] + ArenaParams.MaxMemoryPerBlockSize)
				{
					PoolIndex = ArenaParams.PoolCount; // was in the gap
				}
			}
			else if ((uint8*)Ptr >= PoolBaseVMPtr[(int32)PoolIndex] + ArenaParams.MaxMemoryPerBlockSize)
			{
				do
				{
					PoolIndex++;
					check(PoolIndex < ArenaParams.PoolCount);
				} while ((uint8*)Ptr >= PoolBaseVMPtr[(int32)PoolIndex] + ArenaParams.MaxMemoryPerBlockSize);
				if ((uint8*)Ptr < PoolBaseVMPtr[(int32)PoolIndex])
				{
					PoolIndex = ArenaParams.PoolCount; // was in the gap
				}
			}
		}
		return PoolIndex;
	}

	FORCEINLINE uint8* PoolBasePtr(uint32 InPoolIndex)
	{
		return PoolBaseVMPtr[InPoolIndex];
	}
	FORCEINLINE uint64 PoolIndexFromPtrChecked(const void* Ptr)
	{
		uint64 Result = PoolIndexFromPtr(Ptr);
		check(Result < ArenaParams.PoolCount);
		return Result;
	}

	FORCEINLINE bool IsOSAllocation(const void* Ptr)
	{
		return PoolIndexFromPtr(Ptr) >= ArenaParams.PoolCount;
	}


	FORCEINLINE void* BlockOfBlocksPointerFromContainedPtr(const void* Ptr, uint8 PagesPlatformForBlockOfBlocks, uint32& OutBlockOfBlocksIndex)
	{
		uint32 PoolIndex = PoolIndexFromPtrChecked(Ptr);
		uint8* PoolStart = PoolBasePtr(PoolIndex);
		uint64 BlockOfBlocksIndex = (UPTRINT(Ptr) - UPTRINT(PoolStart)) / (UPTRINT(PagesPlatformForBlockOfBlocks) * UPTRINT(ArenaParams.AllocationGranularity));
		OutBlockOfBlocksIndex = BlockOfBlocksIndex;

		uint8* Result = PoolStart + BlockOfBlocksIndex * UPTRINT(PagesPlatformForBlockOfBlocks) * UPTRINT(ArenaParams.AllocationGranularity);

		check(Result < PoolStart + ArenaParams.MaxMemoryPerBlockSize);
		return Result;
	}
	FORCEINLINE uint8* BlockPointerFromIndecies(uint32 InPoolIndex, uint32 BlockOfBlocksIndex, uint32 BlockOfBlocksSize)
	{
		uint8* PoolStart = PoolBasePtr(InPoolIndex);
		uint8* Ptr = PoolStart + BlockOfBlocksIndex * uint64(BlockOfBlocksSize);
		check(Ptr + BlockOfBlocksSize <= PoolStart + ArenaParams.MaxMemoryPerBlockSize);
		return Ptr;
	}
	FPoolInfoSmall* PushNewPoolToFront(FMallocBinnedArena& Allocator, uint32 InBlockSize, uint32 InPoolIndex, uint32& OutBlockOfBlocksIndex);
	FPoolInfoSmall* GetFrontPool(FPoolTable& Table, uint32 InPoolIndex, uint32& OutBlockOfBlocksIndex);

	FORCEINLINE bool AdjustSmallBlockSizeForAlignment(SIZE_T& InOutSize, uint32 Alignment)
	{
		if ((InOutSize <= ArenaParams.MaxPoolSize) & (Alignment <= ArenaParams.MinimumAlignment)) // one branch, not two
		{
			return true;
		}
		SIZE_T AlignedSize = Align(InOutSize, Alignment);
		if (ArenaParams.bAttemptToAlignSmallBocks & (AlignedSize <= ArenaParams.MaxPoolSize) & (Alignment <= ArenaParams.MaximumAlignmentForSmallBlock)) // one branch, not three
		{
			uint32 PoolIndex = BoundSizeToPoolIndex(AlignedSize);
			while (true)
			{
				uint32 BlockSize = PoolIndexToBlockSize(PoolIndex);
				if (IsAligned(BlockSize, Alignment))
				{
					InOutSize = SIZE_T(BlockSize);
					return true;
				}
				PoolIndex++;
				check(PoolIndex < ArenaParams.PoolCount);
			}
		}
		return false;
	}

public:


	FMallocBinnedArena();
	FArenaParams& GetParams()
	{
		return ArenaParams;
	}
	void InitMallocBinned();

	virtual ~FMallocBinnedArena();


	// FMalloc interface.
	virtual bool IsInternallyThreadSafe() const override;
	FORCEINLINE virtual void* Malloc(SIZE_T Size, uint32 Alignment) override
	{
		Alignment = FMath::Max<uint32>(Alignment, ArenaParams.MinimumAlignment);

		void* Result = nullptr;

		// Only allocate from the small pools if the size is small enough and the alignment isn't crazy large.
		// With large alignments, we'll waste a lot of memory allocating an entire page, but such alignments are highly unlikely in practice.
		if (AdjustSmallBlockSizeForAlignment(Size, Alignment))
		{
			FPerThreadFreeBlockLists* Lists = ArenaParams.bPerThreadCaches ? FPerThreadFreeBlockLists::Get(BinnedArenaTlsSlot) : nullptr;
			if (Lists)
			{
				uint32 PoolIndex = BoundSizeToPoolIndex(Size);
				uint32 BlockSize = PoolIndexToBlockSize(PoolIndex);
				Result = Lists->Malloc(PoolIndex);
				if (Result)
				{
					Lists->AllocatedMemory += BlockSize;
					checkSlow(IsAligned(Result, Alignment));
				}
			}
		}
		if (Result == nullptr)
		{
			Result = MallocExternal(Size, Alignment);
		}

		return Result;
	}
	FORCEINLINE virtual void* Realloc(void* Ptr, SIZE_T NewSize, uint32 Alignment) override
	{
		Alignment = FMath::Max<uint32>(Alignment, ArenaParams.MinimumAlignment);
		if (AdjustSmallBlockSizeForAlignment(NewSize, Alignment))
		{
			FPerThreadFreeBlockLists* Lists = ArenaParams.bPerThreadCaches ? FPerThreadFreeBlockLists::Get(BinnedArenaTlsSlot) : nullptr;

			uint64 PoolIndex = PoolIndexFromPtr(Ptr);
			if ((!!Lists) & ((!Ptr) | (PoolIndex < ArenaParams.PoolCount)))
			{
				uint32 BlockSize = 0;

				bool bCanFree = true; // the nullptr is always "freeable"
				if (Ptr)
				{
					// Reallocate to a smaller/bigger pool if necessary
					BlockSize = PoolIndexToBlockSize(PoolIndex);
					if ((!!NewSize) & (NewSize <= BlockSize) & IsAligned(BlockSize, Alignment) & ((!PoolIndex) | (NewSize > PoolIndexToBlockSize(PoolIndex - 1)))) //-V792
					{
						return Ptr;
					}
					bCanFree = Lists->CanFree(PoolIndex, BlockSize, ArenaParams);
				}
				if (bCanFree)
				{
					uint32 NewPoolIndex = BoundSizeToPoolIndex(NewSize);
					uint32 NewBlockSize = PoolIndexToBlockSize(NewPoolIndex);
					void* Result = NewSize ? Lists->Malloc(NewPoolIndex) : nullptr;
					if (Result)
					{
						Lists->AllocatedMemory += NewBlockSize;
					}
					if (Result || !NewSize)
					{
						if (Result && Ptr)
						{
							FMemory::Memcpy(Result, Ptr, FPlatformMath::Min<SIZE_T>(NewSize, BlockSize));
						}
						if (Ptr)
						{
							bool bDidPush = Lists->Free(Ptr, PoolIndex, BlockSize, ArenaParams);
							checkSlow(bDidPush);
							Lists->AllocatedMemory -= BlockSize;
						}

						return Result;
					}
				}
			}
		}
		void* Result = ReallocExternal(Ptr, NewSize, Alignment);
		return Result;
	}

	FORCEINLINE virtual void Free(void* Ptr) override
	{
		uint64 PoolIndex = PoolIndexFromPtr(Ptr);
		if (PoolIndex < ArenaParams.PoolCount)
		{
			FPerThreadFreeBlockLists* Lists = ArenaParams.bPerThreadCaches ? FPerThreadFreeBlockLists::Get(BinnedArenaTlsSlot) : nullptr;
			if (Lists)
			{
				int32 BlockSize = PoolIndexToBlockSize(PoolIndex);
				if (Lists->Free(Ptr, PoolIndex, BlockSize, ArenaParams))
				{
					Lists->AllocatedMemory -= BlockSize;
					return;
				}
			}
		}
		FreeExternal(Ptr);
	}
	FORCEINLINE virtual bool GetAllocationSize(void *Ptr, SIZE_T &SizeOut) override
	{
		uint64 PoolIndex = PoolIndexFromPtr(Ptr);
		if (PoolIndex < ArenaParams.PoolCount)
		{
			SizeOut = PoolIndexToBlockSize(PoolIndex);
			return true;
		}
		return GetAllocationSizeExternal(Ptr, SizeOut);
	}

	FORCEINLINE virtual SIZE_T QuantizeSize(SIZE_T Count, uint32 Alignment) override
	{
		check(DEFAULT_ALIGNMENT <= ArenaParams.MinimumAlignment); // used below
		checkSlow((Alignment & (Alignment - 1)) == 0); // Check the alignment is a power of two
		SIZE_T SizeOut;
		if ((Count <= ArenaParams.MaxPoolSize) & (Alignment <= ArenaParams.MinimumAlignment)) // one branch, not two
		{
			SizeOut = PoolIndexToBlockSize(BoundSizeToPoolIndex(Count));
		}
		else
		{
			Alignment = FPlatformMath::Max<uint32>(Alignment, ArenaParams.AllocationGranularity);
			SizeOut = Align(Count, Alignment);
		}
		check(SizeOut >= Count);
		return SizeOut;
	}

	virtual bool ValidateHeap() override;
	virtual void Trim(bool bTrimThreadCaches) override;
	virtual void SetupTLSCachesOnCurrentThread() override;
	virtual void ClearAndDisableTLSCachesOnCurrentThread() override;
	virtual const TCHAR* GetDescriptiveName() override;
	// End FMalloc interface.

	void FlushCurrentThreadCache();
	void* MallocExternal(SIZE_T Size, uint32 Alignment);
	void* ReallocExternal(void* Ptr, SIZE_T NewSize, uint32 Alignment);
	void FreeExternal(void *Ptr);
	bool GetAllocationSizeExternal(void* Ptr, SIZE_T& SizeOut);

	MBA_STAT(int64 GetTotalAllocatedSmallPoolMemory();)
	virtual void GetAllocatorStats(FGenericMemoryStats& out_Stats) override;
	/** Dumps current allocator stats to the log. */
	virtual void DumpAllocatorStats(class FOutputDevice& Ar) override;

	FORCEINLINE uint32 BoundSizeToPoolIndex(SIZE_T Size)
	{
		auto Index = ((Size + ArenaParams.MinimumAlignment - 1) >> ArenaParams.MinimumAlignmentShift);
		checkSlow(Index >= 0 && Index <= (ArenaParams.MaxPoolSize >> ArenaParams.MinimumAlignmentShift)); // and it should be in the table
		uint32 PoolIndex = uint32(MemSizeToIndex[Index]);
		checkSlow(PoolIndex >= 0 && PoolIndex < ArenaParams.PoolCount);
		return PoolIndex;
	}
	FORCEINLINE uint32 PoolIndexToBlockSize(uint32 PoolIndex)
	{
		return uint32(SmallBlockSizesReversedShifted[ArenaParams.PoolCount - PoolIndex - 1]) << ArenaParams.MinimumAlignmentShift;
	}

	void Commit(uint32 InPoolIndex, void *Ptr, SIZE_T Size);
	void Decommit(uint32 InPoolIndex, void *Ptr, SIZE_T Size);


	// Pool tables for different pool sizes
	TArray<FPoolTable> SmallPoolTables;

	uint32 SmallPoolInfosPerPlatformPage;

	PoolHashBucket* HashBuckets;
	PoolHashBucket* HashBucketFreeList;
	uint64 NumLargePoolsPerPage;

	FCriticalSection Mutex;
	FGlobalRecycler GGlobalRecycler;
	FPtrToPoolMapping PtrToPoolMapping;

	FArenaParams ArenaParams;

	TArray<uint16> SmallBlockSizesReversedShifted; // this is reversed to get the smallest elements on our main cache line
	uint32 BinnedArenaTlsSlot;
	uint64 PoolSearchDiv; // if this is zero, the VM turned out to be contiguous anyway so we use a simple subtract and shift
	uint8* HighestPoolBaseVMPtr; // this is a duplicate of PoolBaseVMPtr[ArenaParams.PoolCount - 1]
	FPlatformMemory::FPlatformVirtualMemoryBlock PoolBaseVMBlock;
	TArray<uint8*> PoolBaseVMPtr;
	TArray<FPlatformMemory::FPlatformVirtualMemoryBlock> PoolBaseVMBlocks;
	// Mapping of sizes to small table indices
	TArray<uint8> MemSizeToIndex;

	MBA_STAT(
		int64 BinnedArenaAllocatedSmallPoolMemory = 0; // memory that's requested to be allocated by the game
		int64 BinnedArenaAllocatedOSSmallPoolMemory = 0;

		int64 BinnedArenaAllocatedLargePoolMemory = 0; // memory requests to the OS which don't fit in the small pool
		int64 BinnedArenaAllocatedLargePoolMemoryWAlignment = 0; // when we allocate at OS level we need to align to a size

		int64 BinnedArenaPoolInfoMemory = 0;
		int64 BinnedArenaHashMemory = 0;
		int64 BinnedArenaFreeBitsMemory = 0;
		int64 BinnedArenaTLSMemory = 0;
		TAtomic<int64> ConsolidatedMemory;
	)

	FCriticalSection FreeBlockListsRegistrationMutex;
	FCriticalSection& GetFreeBlockListsRegistrationMutex()
	{
		return FreeBlockListsRegistrationMutex;
	}
	TArray<FPerThreadFreeBlockLists*> RegisteredFreeBlockLists;
	TArray<FPerThreadFreeBlockLists*>& GetRegisteredFreeBlockLists()
	{
		return RegisteredFreeBlockLists;
	}
	void RegisterThreadFreeBlockLists(FPerThreadFreeBlockLists* FreeBlockLists)
	{
		FScopeLock Lock(&GetFreeBlockListsRegistrationMutex());
		GetRegisteredFreeBlockLists().Add(FreeBlockLists);
	}
	int64 UnregisterThreadFreeBlockLists(FPerThreadFreeBlockLists* FreeBlockLists)
	{
		FScopeLock Lock(&GetFreeBlockListsRegistrationMutex());
		int64 Result = FreeBlockLists->AllocatedMemory;
		check(Result >= 0);
		GetRegisteredFreeBlockLists().Remove(FreeBlockLists);
		return Result;
	}

	TArray<void*> MallocedPointers;
};

PRAGMA_RESTORE_UNSAFE_TYPECAST_WARNINGS

#endif
