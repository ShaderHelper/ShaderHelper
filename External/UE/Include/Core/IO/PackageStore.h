// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PackageId.h"
#include "Misc/SecureHash.h"

class FArchive;
class FStructuredArchiveSlot;
class FCbObject;
class FCbWriter;

/**
 * Package export information.
 */
struct FPackageStoreExportInfo
{
	int32 ExportCount = 0;
	int32 ExportBundleCount = 0;
	
	CORE_API friend FArchive& operator<<(FArchive& Ar, FPackageStoreExportInfo& ExportInfo);
	
	CORE_API friend FCbWriter& operator<<(FCbWriter& Writer, const FPackageStoreExportInfo& ExportInfo);
	
	CORE_API static FPackageStoreExportInfo FromCbObject(const FCbObject& Obj);
};

/**
 * Package store entry status.
 */
enum class EPackageStoreEntryStatus
{
	None,
	Ok,
	Pending,
	Missing,
};

/**
 * Package store entry.
 */ 
struct FPackageStoreEntry
{
	FPackageStoreExportInfo ExportInfo;
	TArrayView<const FPackageId> ImportedPackageIds;
	TArrayView<const FSHAHash> ShaderMapHashes;
#if WITH_EDITOR
	FName UncookedPackageName;
	uint8 UncookedPackageHeaderExtension; // TODO: Can't include PackagePath.h
#endif
};

/**
 * Package store entry flags.
 */
enum class EPackageStoreEntryFlags : uint32
{
	None		= 0,
	Redirected	= 0x01,
	Optional	= 0x02,
};
ENUM_CLASS_FLAGS(EPackageStoreEntryFlags);

/**
 * Package store entry resource.
 *
 * This is a non-optimized serializable version
 * of a package store entry. Used when cooking
 * and when running cook-on-the-fly.
 */
struct FPackageStoreEntryResource
{
	/** The package store entry flags. */
	EPackageStoreEntryFlags Flags;
	/** The package name. */
	FName PackageName;
	/** Used for localized and redirected packages. */
	FName SourcePackageName;
	/** Region name for localized packages. */
	FName Region;
	/** The package export information. */
	FPackageStoreExportInfo ExportInfo;
	/** Imported package IDs. */
	TArray<FPackageId> ImportedPackageIds;
	/** Referenced shader map hashes. */
	TArray<FSHAHash> ShaderMapHashes;

	/** Returns the package ID. */
	FPackageId GetPackageId() const
	{
		return FPackageId::FromName(PackageName, IsOptional());
	}

	/** Returns the source package ID. */
	FPackageId GetSourcePackageId() const
	{
		return SourcePackageName.IsNone() ? FPackageId() : FPackageId::FromName(SourcePackageName, IsOptional());
	}

	FName GetSourcePackageName() const
	{
		return SourcePackageName;
	}

	/** Returns whether this package is redirected. */
	bool IsRedirected() const
	{
		return EnumHasAnyFlags(Flags, EPackageStoreEntryFlags::Redirected); 
	}

	/** Returns whether this package is optional. */
	bool IsOptional() const
	{
		return EnumHasAnyFlags(Flags, EPackageStoreEntryFlags::Optional);
	}

	CORE_API friend FArchive& operator<<(FArchive& Ar, FPackageStoreEntryResource& PackageStoreEntry);
	
	CORE_API friend FCbWriter& operator<<(FCbWriter& Writer, const FPackageStoreEntryResource& PackageStoreEntry);
	
	CORE_API static FPackageStoreEntryResource FromCbObject(const FCbObject& Obj);
};

/**
 * Stores information about available packages that can be loaded.
 */
class IPackageStore
{
public:
	/* Destructor. */
	virtual ~IPackageStore() { }

	virtual void Initialize() = 0;

	/** Lock the package store for reading */
	virtual void Lock() = 0;

	/** Unlock the package store */
	virtual void Unlock() = 0;

	/* Returns whether the package exists. */
	virtual bool DoesPackageExist(FPackageId PackageId) = 0;

	/* Returns the package store entry data with export info and imported packages for the specified package ID. */
	virtual EPackageStoreEntryStatus GetPackageStoreEntry(FPackageId PackageId, FPackageStoreEntry& OutPackageStoreEntry) = 0;

	/* Returns the redirected package ID and source package name for the specified package ID if it's being redirected. */
	virtual bool GetPackageRedirectInfo(FPackageId PackageId, FName& OutSourcePackageName, FPackageId& OutRedirectedToPackageId) = 0;

	/* Event broadcasted when pending entries are completed and added to the package store */
	DECLARE_EVENT(IPackageStore, FEntriesAddedEvent);
	virtual FEntriesAddedEvent& OnPendingEntriesAdded() = 0;
};

class FPackageStoreBase
	: public IPackageStore
{
public:
	virtual FEntriesAddedEvent& OnPendingEntriesAdded() override
	{
		return PendingEntriesAdded;
	}

protected:
	FEntriesAddedEvent PendingEntriesAdded;
};