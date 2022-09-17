// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Async/Future.h"
#include "Containers/StringView.h"
#include "IO/IoDispatcher.h"
#include "Misc/AssertionMacros.h"
#include "Misc/DateTime.h"
#include "Misc/EnumClassFlags.h"
#include "Misc/SecureHash.h"
#include "Serialization/CompactBinary.h"
#include "Templates/UniquePtr.h"

class FAssetRegistryState;
class FLargeMemoryWriter;
class ICookedPackageWriter;
class IPackageStoreWriter;
struct FPackageStoreEntryResource;
struct FSavePackageArgs;
struct FSavePackageResultStruct;

/** Interface for SavePackage to write packages to storage. */
class IPackageWriter
{
public:
	virtual ~IPackageWriter() = default;

	struct FCapabilities
	{
		/**
		 * Whether an entry should be created for each BulkData stored in the BulkData section
		 * This is necessary for some Writers that need to be able to load the BulkDatas individually.
		 * For other writers the extra regions are an unnecessary performance cost.
		 */
		bool bDeclareRegionForEachAdditionalFile = false;

		/** Applicable only to -diffonly saves; suppresses output and breakpoints for diffs in the header. */
		bool bIgnoreHeaderDiffs = false;
	};

	/** Return capabilities/settings this PackageWriter has/requires 
	  */
	virtual FCapabilities GetCapabilities() const
	{
		return FCapabilities();
	}

	// Events the PackageWriter receives
	struct FBeginPackageInfo
	{
		FName	PackageName;
		FString	LooseFilePath;
	};

	/** Mark the beginning of a package store transaction for the specified package

		This must be called before any data is produced for a given package
	  */
	virtual void BeginPackage(const FBeginPackageInfo& Info) = 0;

	struct FCommitAttachmentInfo
	{
		FUtf8StringView Key;
		FCbObject Value;
	};
	enum class EWriteOptions
	{
		None = 0,
		WritePackage = 0x01,
		WriteSidecars = 0x02,
		Write = WritePackage | WriteSidecars,
		ComputeHash = 0x04,
		SaveForDiff = 0x08,
	};
	struct FCommitPackageInfo
	{
		FName PackageName;
		FGuid PackageGuid;
		TArray<FCommitAttachmentInfo> Attachments;
		bool bSucceeded = false;
		EWriteOptions WriteOptions;
	};

	/** Finalize a package started with BeginPackage()
	  */
	virtual TFuture<FMD5Hash> CommitPackage(FCommitPackageInfo&& Info) = 0;

	struct FPackageInfo
	{
		/** Associated Package Name Entry from BeginPackage */
		FName		InputPackageName;
		/** Output Package Name (Input Package can produce multiple output) */
		FName		OutputPackageName;
		FString		LooseFilePath;
		uint64		HeaderSize = 0;
		FIoChunkId	ChunkId = FIoChunkId::InvalidChunkId;
		uint32		MultiOutputIndex = 0;
	};

	/** Write package data (exports and serialized header)

		This may only be called after a BeginPackage() call has been made
		to signal the start of a package store transaction
	  */
	virtual void WritePackageData(const FPackageInfo& Info, FLargeMemoryWriter& ExportsArchive, const TArray<FFileRegion>& FileRegions) = 0;

	struct FBulkDataInfo
	{
		enum EType
		{
			AppendToExports,
			BulkSegment,
			Mmap,
			Optional,
			NumTypes,
		};

		/** Associated Package Name Entry */
		FName		InputPackageName;
		/** Output Package Name (Input Package can produce multiple output) */
		FName		OutputPackageName;
		EType		BulkDataType = BulkSegment;
		FString		LooseFilePath;
		FIoChunkId	ChunkId = FIoChunkId::InvalidChunkId;
		uint32		MultiOutputIndex = 0;
	};

	/** Write bulk data for the current package
	  */
	virtual void WriteBulkData(const FBulkDataInfo& Info, const FIoBuffer& BulkData, const TArray<FFileRegion>& FileRegions) = 0;

	struct FAdditionalFileInfo
	{
		/** Associated Package Name Entry */
		FName		InputPackageName;
		/** Output Package Name (Input Package can produce multiple output) */
		FName		OutputPackageName;
		FString		Filename;
		FIoChunkId	ChunkId;
		uint32		MultiOutputIndex = 0;
	};

	/** Write separate files written by UObjects during cooking via UObject::CookAdditionalFiles. */
	virtual void WriteAdditionalFile(const FAdditionalFileInfo& Info, const FIoBuffer& FileData) = 0;

	struct FLinkerAdditionalDataInfo
	{
		/** Associated Package Name Entry */
		FName	InputPackageName;
		/** Output Package Name (Input Package can produce multiple output) */
		FName	OutputPackageName;
		uint32	MultiOutputIndex = 0;
	};
	/** Write separate data written by UObjects via FLinkerSave::AdditionalDataToAppend. */
	virtual void WriteLinkerAdditionalData(const FLinkerAdditionalDataInfo& Info, const FIoBuffer& Data, const TArray<FFileRegion>& FileRegions) = 0;

	/** Increase the referenced ExportsSize by the size in bytes of the data that will be added on to it during
	 * commit before writing to disk. Used for accurate disk size reporting on the UPackage and AssetRegistry.
	 */
	virtual void AddToExportsSize(int64& InOutExportsSize)
	{
	}

	/** Create the FLargeMemoryWriter to which the Header and Exports are written during the save. */
	COREUOBJECT_API virtual TUniquePtr<FLargeMemoryWriter> CreateLinkerArchive(FName PackageName, UObject* Asset);

	/** Report whether PreSave was already called by the PackageWriter before the current UPackage::Save call. */
	virtual bool IsPreSaveCompleted() const
	{
		return false;
	}

	/** Downcast function for IPackageWriters that implement the ICookedPackageWriters inherited interface. */
	virtual ICookedPackageWriter* AsCookedPackageWriter()
	{
		return nullptr;
	}
};

ENUM_CLASS_FLAGS(IPackageWriter::EWriteOptions);

/** Interface for cooking that writes cooked packages to storage usable by the runtime game. */
class ICookedPackageWriter : public IPackageWriter
{
public:
	virtual ~ICookedPackageWriter() = default;

	struct FCookCapabilities
	{
		/** Whether this writer implements -diffonly and -linkerdiff. */
		bool bDiffModeSupported = false;
	};

	/** Return cook capabilities/settings this PackageWriter has/requires
	  */
	virtual FCookCapabilities GetCookCapabilities() const
	{
		return FCookCapabilities();
	}

	/** Return the timestamp of the previous cook, or FDateTime::MaxValue to indicate previous cook should be assumed newer than any other cook data. */
	virtual FDateTime GetPreviousCookTime() const
	{
		return FDateTime::MaxValue();
	}

	virtual ICookedPackageWriter* AsCookedPackageWriter() override
	{
		return this;
	}

	struct FCookInfo
	{
		enum ECookMode
		{
			CookByTheBookMode,
			CookOnTheFlyMode
		};

		ECookMode CookMode = CookByTheBookMode;
		bool bFullBuild = true;
		bool bIterateSharedBuild = false;
	};

	/** Delete outdated cooked data, etc.
	  */
	virtual void Initialize(const FCookInfo& Info) = 0;

	/** Signal the start of a cooking pass

		Package data may only be produced after BeginCook() has been called and
		before EndCook() is called
	  */
	virtual void BeginCook() = 0;

	/** Signal the end of a cooking pass.
	  */
	virtual void EndCook() = 0;

	/**
	 * Flush any outstanding writes.
	 */
	virtual void Flush() = 0;

	struct FCookedPackageInfo
	{
		FName PackageName;
		FMD5Hash Hash;
		FGuid PackageGuid;
		int64 DiskSize = -1;
	};

	/**
	 * Returns an AssetRegistry describing the previous cook results.
	 */
	virtual TUniquePtr<FAssetRegistryState> LoadPreviousAssetRegistry() = 0;

	/**
	 * Returns an Attachment that was previously commited for the given PackageName.
	 * Returns an empty object if not found.
	 */
	virtual FCbObject GetOplogAttachment(FName PackageName, FUtf8StringView AttachmentKey) = 0;

	/**
	 * Remove the given cooked package(s) from storage; they have been modified since the last cook.
	 */
	virtual void RemoveCookedPackages(TArrayView<const FName> PackageNamesToRemove) = 0;

	/**
	 * Remove all cooked packages from storage.
	 */
	virtual void RemoveCookedPackages() = 0;

	/**
	 * Signal the given cooked package(s) have been checked for changes and have not been modified since the last cook.
	 */
	virtual void MarkPackagesUpToDate(TArrayView<const FName> UpToDatePackages) = 0;

	struct FPreviousCookedBytesData
	{
		TUniquePtr<uint8> Data;
		int64 Size;
		int64 HeaderSize;
		int64 StartOffset;
	};
	/** Load the bytes of the previously-cooked package, used for diffing */
	virtual bool GetPreviousCookedBytes(const FPackageInfo& Info, FPreviousCookedBytesData& OutData)
	{
		// The subclass must override this method if it returns bDiffModeSupported=true in GetCookCapabilities
		unimplemented();
		return false;
	}
	/** Append all data to the Exports archive that would normally be done in CommitPackage, used for diffing. */
	virtual void CompleteExportsArchiveForDiff(const FPackageInfo& Info, FLargeMemoryWriter& ExportsArchive)
	{
		// The subclass must override this method if it returns bDiffModeSupported=true in GetCookCapabilities
		unimplemented();
	}
	/** Modify the SaveArgs if required before the first Save. Used for diffing. */
	virtual void UpdateSaveArguments(FSavePackageArgs& SaveArgs)
	{
	}
	/** Report whether an additional save is needed and set up for it if so. Used for diffing. */
	virtual bool IsAnotherSaveNeeded(FSavePackageResultStruct& PreviousResult, FSavePackageArgs& SaveArgs)
	{
		return false;
	}
	/** Downcast function for ICookedPackageWriters that implement the IPackageStoreWriter inherited interface. */
	virtual IPackageStoreWriter* AsPackageStoreWriter()
	{
		return nullptr;
	}
};

static inline const ANSICHAR* LexToString(IPackageWriter::FBulkDataInfo::EType Value)
{
	switch (Value)
	{
	case IPackageWriter::FBulkDataInfo::AppendToExports:
		return "AppendToExports";
	case IPackageWriter::FBulkDataInfo::BulkSegment:
		return "Standard";
	case IPackageWriter::FBulkDataInfo::Mmap:
		return "Mmap";
	case IPackageWriter::FBulkDataInfo::Optional:
		return "Optional";
	default:
		checkNoEntry();
		return "Unknown";
	}

}
