// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AsyncLoading2.h: Unreal async loading #2 definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectResource.h"
#include "IO/PackageId.h"
#include "Serialization/Archive.h"
#include "Serialization/MappedName.h"
#include "Serialization/CustomVersion.h"

class FArchive;
class IAsyncPackageLoader;
class FIoDispatcher;
class IEDLBootNotificationManager;

class FPackageImportReference
{
public:
	FPackageImportReference(uint32 InImportedPackageIndex, uint32 InImportedPublicExportHashIndex)
		: ImportedPackageIndex(InImportedPackageIndex)
		, ImportedPublicExportHashIndex(InImportedPublicExportHashIndex)
	{
	}

	uint32 GetImportedPackageIndex() const
	{
		return ImportedPackageIndex;
	}

	uint32 GetImportedPublicExportHashIndex() const
	{
		return ImportedPublicExportHashIndex;
	}

private:
	uint32 ImportedPackageIndex;
	uint32 ImportedPublicExportHashIndex;
};

class FPackageObjectIndex
{
	static constexpr uint64 IndexBits = 62ull;
	static constexpr uint64 IndexMask = (1ull << IndexBits) - 1ull;
	static constexpr uint64 TypeMask = ~IndexMask;
	static constexpr uint64 TypeShift = IndexBits;
	static constexpr uint64 Invalid = ~0ull;

	uint64 TypeAndId = Invalid;

	enum EType
	{
		Export,
		ScriptImport,
		PackageImport,
		Null,
		TypeCount = Null,
	};
	static_assert((TypeCount - 1) <= (TypeMask >> TypeShift), "FPackageObjectIndex: Too many types for TypeMask");

	inline explicit FPackageObjectIndex(EType InType, uint64 InId) : TypeAndId((uint64(InType) << TypeShift) | InId) {}

	COREUOBJECT_API static uint64 GenerateImportHashFromObjectPath(const FStringView& ObjectPath);

public:
	FPackageObjectIndex() = default;

	inline static FPackageObjectIndex FromExportIndex(const int32 Index)
	{
		return FPackageObjectIndex(Export, Index);
	}

	inline static FPackageObjectIndex FromScriptPath(const FStringView& ScriptObjectPath)
	{
		return FPackageObjectIndex(ScriptImport, GenerateImportHashFromObjectPath(ScriptObjectPath));
	}

	inline static FPackageObjectIndex FromPackageImportRef(const FPackageImportReference& PackageImportRef)
	{
		uint64 Id = static_cast<uint64>(PackageImportRef.GetImportedPackageIndex()) << 32 | PackageImportRef.GetImportedPublicExportHashIndex();
		check(!(Id & TypeMask));
		return FPackageObjectIndex(PackageImport, Id);
	}

	inline bool IsNull() const
	{
		return TypeAndId == Invalid;
	}

	inline bool IsExport() const
	{
		return (TypeAndId >> TypeShift) == Export;
	}

	inline bool IsImport() const
	{
		return IsScriptImport() || IsPackageImport();
	}

	inline bool IsScriptImport() const
	{
		return (TypeAndId >> TypeShift) == ScriptImport;
	}

	inline bool IsPackageImport() const
	{
		return (TypeAndId >> TypeShift) == PackageImport;
	}

	inline uint32 ToExport() const
	{
		check(IsExport());
		return uint32(TypeAndId);
	}

	inline FPackageImportReference ToPackageImportRef() const
	{
		uint32 ImportedPackageIndex = static_cast<uint32>((TypeAndId & IndexMask) >> 32);
		uint32 ExportHash = static_cast<uint32>(TypeAndId);
		return FPackageImportReference(ImportedPackageIndex, ExportHash);
	}

	inline uint64 Value() const
	{
		return TypeAndId & IndexMask;
	}

	inline bool operator==(FPackageObjectIndex Other) const
	{
		return TypeAndId == Other.TypeAndId;
	}

	inline bool operator!=(FPackageObjectIndex Other) const
	{
		return TypeAndId != Other.TypeAndId;
	}

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FPackageObjectIndex& Value)
	{
		Ar << Value.TypeAndId;
		return Ar;
	}

	inline friend uint32 GetTypeHash(const FPackageObjectIndex& Value)
	{
		return uint32(Value.TypeAndId);
	}
};

class FPublicExportKey
{
public:
	FPublicExportKey()
	{
	}

	bool IsNull() const
	{
		return GetExportHash() == 0;
	}

	FPackageId GetPackageId() const
	{
		return FPackageId::FromValue(uint64(PackageIdHigh) << 32 | PackageIdLow);
	}

	uint64 GetExportHash() const
	{
		return uint64(ExportHashHigh) << 32 | ExportHashLow;
	}

	inline bool operator==(const FPublicExportKey& Other) const
	{
		return GetExportHash() == Other.GetExportHash() &&
			GetPackageId() == Other.GetPackageId();
	}

	inline friend uint32 GetTypeHash(const FPublicExportKey& In)
	{
		return HashCombine(GetTypeHash(In.GetPackageId()), GetTypeHash(In.GetExportHash()));
	}

	static FPublicExportKey MakeKey(FPackageId PackageId, uint64 ExportHash)
	{
		check(PackageId.IsValid());
		check(ExportHash);
		uint64 PackageIdValue = PackageId.Value();
		return FPublicExportKey(uint32(PackageIdValue >> 32), uint32(PackageIdValue), uint32(ExportHash >> 32), uint32(ExportHash));
	}

	static FPublicExportKey FromPackageImport(FPackageObjectIndex ObjectIndex, const TArrayView<const FPackageId>& ImportedPackageIds, const TArrayView<const uint64>& ImportedPublicExportHashes)
	{
		check(ObjectIndex.IsPackageImport());
		FPackageImportReference PackageImportRef = ObjectIndex.ToPackageImportRef();
		FPackageId PackageId = ImportedPackageIds[PackageImportRef.GetImportedPackageIndex()];
		uint64 ExportHash = ImportedPublicExportHashes[PackageImportRef.GetImportedPublicExportHashIndex()];
		return MakeKey(PackageId, ExportHash);
	}

private:
	FPublicExportKey(uint32 InPackageIdHigh, uint32 InPackageIdLow, uint32 InExportHashHigh, uint32 InExportHashLow)
		: PackageIdHigh(InPackageIdHigh)
		, PackageIdLow(InPackageIdLow)
		, ExportHashHigh(InExportHashHigh)
		, ExportHashLow(InExportHashLow)
	{
	}

	uint32 PackageIdHigh = 0;
	uint32 PackageIdLow = 0;
	uint32 ExportHashHigh = 0;
	uint32 ExportHashLow = 0;
};

/**
 * Export filter flags.
 */
enum class EExportFilterFlags : uint8
{
	None,
	NotForClient,
	NotForServer
};

enum class EZenPackageVersion : uint32
{
	Initial,

	LatestPlusOne,
	Latest = LatestPlusOne - 1
};

struct FZenPackageVersioningInfo
{
	EZenPackageVersion ZenVersion;
	FPackageFileVersion PackageVersion;
	int32 LicenseeVersion;
	FCustomVersionContainer CustomVersions;

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FZenPackageVersioningInfo& ExportBundleEntry);
};

/**
 * Package summary.
 */
struct FZenPackageSummary
{
	uint32 bHasVersioningInfo;
	uint32 HeaderSize;
	FMappedName Name;
	uint32 PackageFlags;
	uint32 CookedHeaderSize;
	int32 ImportedPublicExportHashesOffset;
	int32 ImportMapOffset;
	int32 ExportMapOffset;
	int32 ExportBundleEntriesOffset;
	int32 GraphDataOffset;
};

/**
 * Export bundle entry.
 */
struct FExportBundleEntry
{
	enum EExportCommandType
	{
		ExportCommandType_Create,
		ExportCommandType_Serialize,
		ExportCommandType_Count
	};
	uint32 LocalExportIndex;
	uint32 CommandType;

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FExportBundleEntry& ExportBundleEntry);
};

/**
 * Export bundle header
 */
struct FExportBundleHeader
{
	uint64 SerialOffset;
	uint32 FirstEntryIndex;
	uint32 EntryCount;

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FExportBundleHeader& ExportBundleHeader);
};

struct FScriptObjectEntry
{
	FMinimalName ObjectName;
	FPackageObjectIndex GlobalIndex;
	FPackageObjectIndex OuterIndex;
	FPackageObjectIndex CDOClassIndex;

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FScriptObjectEntry& ScriptObjectEntry);
};

/**
 * Export map entry.
 */
struct FExportMapEntry
{
	uint64 CookedSerialOffset = 0;
	uint64 CookedSerialSize = 0;
	FMappedName ObjectName;
	FPackageObjectIndex OuterIndex;
	FPackageObjectIndex ClassIndex;
	FPackageObjectIndex SuperIndex;
	FPackageObjectIndex TemplateIndex;
	uint64 PublicExportHash;
	EObjectFlags ObjectFlags = EObjectFlags::RF_NoFlags;
	EExportFilterFlags FilterFlags = EExportFilterFlags::None;
	uint8 Pad[3] = {};

	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FExportMapEntry& ExportMapEntry);
};

COREUOBJECT_API void FindAllRuntimeScriptPackages(TArray<UPackage*>& OutPackages);

#ifndef WITH_ASYNCLOADING2
#define WITH_ASYNCLOADING2 (WITH_IOSTORE_IN_EDITOR || !WITH_EDITORONLY_DATA)
#endif

#if WITH_ASYNCLOADING2

/**
 * Creates a new instance of the AsyncPackageLoader #2.
 *
 * @param InIoDispatcher				The I/O dispatcher.
 *
 * @return The async package loader.
 */
IAsyncPackageLoader* MakeAsyncPackageLoader2(FIoDispatcher& InIoDispatcher, IAsyncPackageLoader* UncookedPackageLoader = nullptr);

#endif
