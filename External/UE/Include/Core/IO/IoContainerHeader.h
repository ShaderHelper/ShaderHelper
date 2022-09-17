// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IO/PackageId.h"
#include "IO/IoContainerId.h"
#include "Serialization/MappedName.h"

class FArchive;

/**
 * Package store entry array view.
 */
template<typename T>
class TFilePackageStoreEntryCArrayView
{
	const uint32 ArrayNum = 0;
	const uint32 OffsetToDataFromThis = 0;

public:
	inline uint32 Num() const { return ArrayNum; }

	inline const T* Data() const { return (T*)((char*)this + OffsetToDataFromThis); }
	inline T* Data() { return (T*)((char*)this + OffsetToDataFromThis); }

	inline const T* begin() const { return Data(); }
	inline T* begin() { return Data(); }

	inline const T* end() const { return Data() + ArrayNum; }
	inline T* end() { return Data() + ArrayNum; }

	inline const T& operator[](uint32 Index) const { return Data()[Index]; }
	inline T& operator[](uint32 Index) { return Data()[Index]; }
};

/**
 * File based package store entry
 */
struct FFilePackageStoreEntry
{
	int32 ExportCount;
	int32 ExportBundleCount;
	TFilePackageStoreEntryCArrayView<FPackageId> ImportedPackages;
	TFilePackageStoreEntryCArrayView<FSHAHash> ShaderMapHashes;
};

struct FIoContainerHeaderPackageRedirect
{
	FPackageId SourcePackageId;
	FPackageId TargetPackageId;
	FMappedName SourcePackageName;

	CORE_API friend FArchive& operator<<(FArchive& Ar, FIoContainerHeaderPackageRedirect& PackageRedirect);
};

struct FIoContainerHeaderLocalizedPackage
{
	FPackageId SourcePackageId;
	FMappedName SourcePackageName;

	CORE_API friend FArchive& operator<<(FArchive& Ar, FIoContainerHeaderLocalizedPackage& LocalizedPackage);
};

enum class EIoContainerHeaderVersion : uint32
{
	Initial = 0,
	LocalizedPackages = 1,

	LatestPlusOne,
	Latest = LatestPlusOne - 1
};

struct FIoContainerHeader
{
	enum
	{
		Signature = 0x496f436e
	};

	FIoContainerId ContainerId;
	uint32 PackageCount = 0;
	TArray<FPackageId> PackageIds;
	TArray<uint8> StoreEntries; //FPackageStoreEntry[PackageCount]
	TArray<FNameEntryId> RedirectsNameMap;
	TArray<FIoContainerHeaderLocalizedPackage> LocalizedPackages;
	TArray<FIoContainerHeaderPackageRedirect> PackageRedirects;

	CORE_API friend FArchive& operator<<(FArchive& Ar, FIoContainerHeader& ContainerHeader);
};
