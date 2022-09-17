// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "HAL/Platform.h"
#include "Serialization/Archive.h"
#include "Serialization/LargeMemoryWriter.h"
#include "Serialization/LargeMemoryReader.h"
#include "Templates/RefCounting.h"
#include "Templates/UniquePtr.h"
#include "UObject/NameTypes.h"

class FUObjectThreadContext;
class FLinkerLoad;
class FProperty;
class UObject;
struct FObjectImport;
struct FObjectExport;

/** Structure that holds stats from comparing two packages */
struct COREUOBJECT_API FArchiveDiffStats
{
	/** Size of all of the differences between two packages */
	int64 DiffSize;
	/** Number of differences between two packages */
	int64 NumDiffs;
	/** Size of the source package file (the one we compared against) */
	int64 OriginalFileTotalSize;
	/** Size of the new package file */
	int64 NewFileTotalSize;

	FArchiveDiffStats()
		: DiffSize(0)
		, NumDiffs(0)
		, OriginalFileTotalSize(0)
		, NewFileTotalSize(0)
	{}
};

class COREUOBJECT_API FArchiveStackTraceIgnoreScope
{
	const bool bIgnore;
public:
	FArchiveStackTraceIgnoreScope(bool bInIgnore = true);
	~FArchiveStackTraceIgnoreScope();
};

struct COREUOBJECT_API FArchiveDiffInfo
{
	int64 Offset;
	int64 Size;
	FArchiveDiffInfo()
		: Offset(0)
		, Size(0)
	{
	}
	FArchiveDiffInfo(int64 InOffset, int64 InSize)
		: Offset(InOffset)
		, Size(InSize)
	{
	}
	bool operator==(const FArchiveDiffInfo& InOther) const
	{
		return Offset == InOther.Offset;
	}
	bool operator<(const FArchiveDiffInfo& InOther) const
	{
		return Offset < InOther.Offset;
	}
	friend FArchive& operator << (FArchive& Ar, FArchiveDiffInfo& InDiffInfo)
	{
		Ar << InDiffInfo.Offset;
		Ar << InDiffInfo.Size;
		return Ar;
	}
};

class COREUOBJECT_API FArchiveDiffMap : public TArray<FArchiveDiffInfo>
{

};

/**
 * Archive that stores a callstack for each of the Serialize calls and has the ability to compare itself to an existing
 * package on disk and dump all the differences to log.
 */
class COREUOBJECT_API FArchiveStackTrace : public FLargeMemoryWriter
{
	/** Offset and callstack pair */
	struct FCallstactAtOffset
	{
		/** Offset of a Serialize call */
		int64 Offset;
		/** Callstack CRC for the Serialize call */
		uint32 Callstack;
		/** Collected inside of skip scope */
		uint32 bIgnore : 1;

		FCallstactAtOffset()
			: Offset(-1)
			, Callstack(0)
			, bIgnore(false)
		{}

		FCallstactAtOffset(int64 InOffset, uint32 InCallstack, bool bInIgnore)
			: Offset(InOffset)
			, Callstack(InCallstack)
			, bIgnore(bInIgnore)
		{
		}
	};

	/** Struct to hold the actual Serialize call callstack and any associated data */
	struct FCallstackData
	{
		/** Full callstack */
		ANSICHAR* Callstack;
		/** Full name of the currently serialized object */
		FString SerializedObjectName;
		/** The currently serialized property */
		FProperty* SerializedProp;
		/** Name of the currently serialized property */
		FString SerializedPropertyName;

		FCallstackData();
		FCallstackData(ANSICHAR* InCallstack, UObject* InSerializedObject, FProperty* InSerializedProperty);

		/** Converts the callstack and associated data to human readable string */
		FString ToString(const TCHAR* CallstackCutoffText) const;
	};

	/** The asset being serialized */
	UObject* Asset;
	/** The class of asset being serialized */
	FName AssetClass;

	/** List of offsets and their respective callstacks */
	TArray<FCallstactAtOffset> CallstackAtOffsetMap;
	/** Contains all unique callstacks for all Serialize calls */
	TMap<uint32, FCallstackData> UniqueCallstacks;
	/** Contains offsets to gather callstacks for */
	const FArchiveDiffMap* DiffMap;
	/** If true the archive will callect callstacks for all offsets or for offsets specified in the DiffMap */
	bool bCollectCallstacks;
	/** Optimizes callstack comparison. If true the current and the last callstack should be compared as it may have changed */
	bool bCallstacksDirty;

	/** Maximum size of the stack trace */
	const SIZE_T StackTraceSize;
	/** Buffer for getting the current stack trace */
	ANSICHAR* StackTrace;
	/** Callstack associated with the previous Serialize call */
	ANSICHAR* LastSerializeCallstack;
	/** Cached thread context */
	FUObjectThreadContext& ThreadContext;
	/** Current save/load context */
	TRefCountPtr<FUObjectSerializeContext> LoadContext;

#if WITH_EDITOR
	/** Current debug name stack */
	TArray<FName> DebugDataStack;
#endif

	/** Adds a unique callstack to UniqueCallstacks map */
	ANSICHAR* AddUniqueCallstack(UObject* InSerializedObject, FProperty* InSerializedProperty, uint32& OutCallstackCRC);

	/** Finds a callstack associated with data at the specified offset */
	int32 GetCallstackAtOffset(int64 InOffset, int32 MinOffsetIndex);

	int64 GetSerializedDataSizeForOffsetIndex(int32 InOffsetIndex)
	{
		if (InOffsetIndex < CallstackAtOffsetMap.Num() - 1)
		{
			return CallstackAtOffsetMap[InOffsetIndex + 1].Offset - CallstackAtOffsetMap[InOffsetIndex].Offset;
		}
		else
		{
			return TotalSize() - CallstackAtOffsetMap[InOffsetIndex].Offset;
		}
	}

	bool ShouldLogOffset(int64 InOffset) const
	{
		if (!DiffMap)
		{
			return true;
		}

		for (const FArchiveDiffInfo& Diff : *DiffMap)
		{
			if (Diff.Offset <= InOffset && InOffset < (Diff.Offset + Diff.Size))
			{
				return true;
			}
		}
		return false;
	}

public:

	struct FPackageData
	{
		uint8* Data;
		int64 Size;
		int64 HeaderSize;
		int64 StartOffset;

		FPackageData()
			: Size(0)
			, HeaderSize(0)
			, StartOffset(0)
		{}
	};

private:

	void CompareWithInternal(const FPackageData& SourcePackage, const FPackageData& DestPackage, const TCHAR* AssetFilename, const TCHAR* CallstackCutoffText, const int64 MaxDiffsToLog, int32& InOutDiffsLogged, TMap<FName, FArchiveDiffStats>& OutStats);
	bool GenerateDiffMapInternal(const FPackageData& SourcePackage, const FPackageData& DestPackage, int32 MaxDiffsToFind, FArchiveDiffMap& OutDiffMap);
	void DumpPackageHeaderDiffs(const FPackageData& SourcePackage, const FPackageData& DestPackage, const FString& AssetFilename, const int32 MaxDiffsToLog);
	static FLinkerLoad* CreateLinkerForPackage(FUObjectSerializeContext* LoadContext, const FString& InPackageName, const FString& InFilename, const FPackageData& PackageData);

public:

	FArchiveStackTrace(UObject* InAsset, const TCHAR* InFilename, bool bInCollectCallstacks = true, const FArchiveDiffMap* InDiffMap = nullptr);
	virtual ~FArchiveStackTrace();

	//~ Begin FArchive Interface
	virtual void Serialize(void* Data, int64 Num) override;
	virtual void SetSerializeContext(FUObjectSerializeContext* InLoadContext) override;
	virtual FUObjectSerializeContext* GetSerializeContext() override;
#if WITH_EDITOR
	virtual void PushDebugDataString(const FName& DebugData) override
	{
		DebugDataStack.Add(DebugData);
	}
	virtual void PopDebugDataString() override
	{
		DebugDataStack.Pop();
	}
#endif
	//~ End FArchive Interface

	/** Compares this archive with the given bytes from disk or FPackageData. Dumps all differences to log. */
	void CompareWith(const TCHAR* InFilename, const int64 TotalHeaderSize, const TCHAR* CallstackCutoffText,
		const int32 MaxDiffsToLog, TMap<FName, FArchiveDiffStats>& OutStats);
	void CompareWith(const FPackageData& SourcePackage, const TCHAR* FileDisplayName, const int64 TotalHeaderSize,
		const TCHAR* CallstackCutoffText, const int32 MaxDiffsToLog, TMap<FName, FArchiveDiffStats>& OutStats);

	/** Generates a map of all differences between this archive and the given bytes from disk or FPackageData. */
	bool GenerateDiffMap(const TCHAR* InFilename, int64 TotalHeaderSize, int32 MaxDiffsToFind, FArchiveDiffMap& OutDiffMap);
	bool GenerateDiffMap(const FPackageData& SourcePackage, int64 TotalHeaderSize, int32 MaxDiffsToFind,
		FArchiveDiffMap& OutDiffMap);

	/** Compares the provided buffer with the given bytes from disk or FPackageData. */
	static bool IsIdentical(const TCHAR* InFilename, int64 BufferSize, const uint8* BufferData);
	static bool IsIdentical(const FPackageData& SourcePackage, int64 BufferSize, const uint8* BufferData);

	/** Helper function to load package contents into memory. Supports EDL packages. */
	static bool LoadPackageIntoMemory(const TCHAR* InFilename, FPackageData& OutPackageData,
		TUniquePtr<uint8>& OutLoadedBytes);
};

class COREUOBJECT_API FArchiveStackTraceReader : public FLargeMemoryReader
{
public:
	struct FSerializeData
	{
		FSerializeData()
			: Offset(0)
			, Size(0)
			, Count(0)
			, Object(nullptr)
			, PropertyName(NAME_None)
		{}
		FSerializeData(int64 InOffset, int64 InSize, UObject* InObject, FProperty* InProperty);
		int64 Offset;
		int64 Size;
		int64 Count;
		UObject* Object;
		FName PropertyName;
		FString FullPropertyName;

		bool IsContiguousSerialization(const FSerializeData& Other) const
		{
			// Return whether this and other are neighboring bits of data for the serialization of the same instance of an object\property
			return Object == Other.Object && PropertyName == Other.PropertyName &&
				(Offset == Other.Offset || (Offset + Size) == Other.Offset); // This is to merge contiguous blocks
		}
	};
private:
	TArray<FSerializeData> SerializeTrace;
	/** Cached thread context */
	FUObjectThreadContext& ThreadContext;
public:

	FArchiveStackTraceReader(const TCHAR* InFilename, const uint8* InData, const int64 Num);

	virtual void Serialize(void* OutData, int64 Num) override;
	const TArray<FSerializeData>& GetSerializeTrace() const
	{
		return SerializeTrace;
	}
	static FArchiveStackTraceReader* CreateFromFile(const TCHAR* InFilename);
};