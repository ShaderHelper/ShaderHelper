// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Compression/CompressedBuffer.h"
#include "Serialization/ArchiveUObject.h"
#include "Serialization/FileRegions.h"
#include "Templates/RefCounting.h"
#include "UObject/Linker.h"
#include "UObject/ObjectResource.h"
#include "UObject/PackageTrailer.h"
#include "UObject/UObjectThreadContext.h"

class FObjectPostSaveContext;
struct FUntypedBulkData;

/*----------------------------------------------------------------------------
	FLinkerSave.
----------------------------------------------------------------------------*/

/**
 * Handles saving Unreal package files.
 */
class FLinkerSave : public FLinker, public FArchiveUObject
{
public:

	FORCEINLINE static ELinkerType::Type StaticType()
	{
		return ELinkerType::Save;
	}

	virtual ~FLinkerSave();

	// Variables.
	/** The archive that actually writes the data to disk. */
	FArchive* Saver;

	FPackageIndex CurrentlySavingExport;
	TArray<FPackageIndex> DepListForErrorChecking;

	/** Index array - location of the resource for a UObject is stored in the ObjectIndices array using the UObject's Index */
	TMap<UObject *,FPackageIndex> ObjectIndicesMap;

	/** List of Searchable Names, by object containing them. This gets turned into package indices later */
	TMap<const UObject *, TArray<FName> > SearchableNamesObjectMap;

	/** Index array - location of the name in the NameMap array for each FName is stored in the NameIndices array using the FName's Index */
	TMap<FNameEntryId, int32> NameIndices;

	/** Save context associated with this linker */
	TRefCountPtr<FUObjectSerializeContext> SaveContext;

	/** List of bulkdata that needs to be stored at the end of the file */
	struct FBulkDataStorageInfo
	{
		/** Offset to the location where the payload offset is stored */
		int64 BulkDataOffsetInFilePos;
		/** Offset to the location where the payload size is stored */
		int64 BulkDataSizeOnDiskPos;
		/** Offset to the location where the bulk data flags are stored */
		int64 BulkDataFlagsPos;
		/** Bulk data flags at the time of serialization */
		uint32 BulkDataFlags;
		/** The file region type to apply to this bulk data */
		EFileRegionType BulkDataFileRegionType;
		/** The bulkdata */
		FUntypedBulkData* BulkData;
	};
	TArray<FBulkDataStorageInfo> BulkDataToAppend;
	TArray<FFileRegion> FileRegions;

	// TODO: Look into removing BulkDataToAppend and use AdditionalDataToAppend instead.

	/**
	 * Callback for arbitrary serializers to append data to the end of the ExportsArchive.
	 * Some PackageWriters used by SavePackage will write this data to a separate archive.
	 * 
	 * @param ExportsArchive The archive containing the UObjects and structs, this is always this LinkerSave.
	 * @param DataArchive The archive to which the data should be written. Might be this LinkerSave, or might be a separate archive.
	 * @param DataStartOffset The offset to the beginning of the range in DataArchive that should be stored in the UObject or struct's
	 *        export data. Reading at DataStartOffset from the FArchive passed into Serialize during a load will return the data that the
	 *		  callback wrote to DataArchive.
	 */
	using AdditionalDataCallback = TUniqueFunction<void(FLinkerSave& ExportsArchive, FArchive& DataArchive, int64 DataStartOffset)>;
	/** 
	 * Array of callbacks that will be invoked when it is possible to serialize out data 
	 * to the end of the output file.
	 */
	TArray<AdditionalDataCallback> AdditionalDataToAppend;

	/**
	 * Set to true when the package is being saved due to a procedural save.
	 * Any save without the the possibility of user-generated edits to the package is a procedural save (Cooking, EditorDomain).
	 * This allows us to execute transforms that only need to be executed in response to new user data.
	 */
	bool bProceduralSave = false;

	/**
	 * Set to true when the LoadedPath of the package being saved is being updated.
	 * This allows us to update the in-memory package when it is saved in editor to match its new save file.
	 * This is used e.g. to decide whether to update the in-memory file offsets for BulkData.
	 */
	bool bUpdatingLoadedPath = false;
	
	struct FSidecarStorageInfo
	{
		FIoHash Identifier;
		FCompressedBuffer Payload;
	};

	/** Used by FEditorBulkData to add payloads to be added to the payload sidecar file (currently an experimental feature) */
	TArray<FSidecarStorageInfo> SidecarDataToAppend;
	
	/** Gathers all payloads while save the package, so that they can be stored in a single data structure @see FPackageTrailer */
	TUniquePtr<UE::FPackageTrailerBuilder> PackageTrailerBuilder;

	/** 
	 * Array of callbacks that will be invoked when the package has successfully saved to disk.
	 * The callbacks will not be invoked if the package fails to save for some reason.
	 * Unlike subscribing to the UPackage::PackageSavedEvent this callback allows custom data
	 * via lambda capture. 
	 * @param PackagePath The path of the package
	 */
	TArray<TUniqueFunction<void(const FPackagePath& PackagePath, FObjectPostSaveContext ObjectSaveContext)>> PostSaveCallbacks;

	/** A mapping of package name to generated script SHA keys */
	COREUOBJECT_API static TMap<FString, TArray<uint8> > PackagesToScriptSHAMap;

	/** Constructor for file writer */
	FLinkerSave(UPackage* InParent, const TCHAR* InFilename, bool bForceByteSwapping, bool bInSaveUnversioned = false );
	/** Constructor for memory writer */
	FLinkerSave(UPackage* InParent, bool bForceByteSwapping, bool bInSaveUnversioned = false );
	/** Constructor for custom savers. The linker assumes ownership of the custom saver. */
	FLinkerSave(UPackage* InParent, FArchive *InSaver, bool bForceByteSwapping, bool bInSaveUnversioned = false);

	/** Returns the appropriate name index for the source name, or 0 if not found in NameIndices */
	int32 MapName( FNameEntryId Name) const;

	/** Returns the appropriate package index for the source object, or default value if not found in ObjectIndicesMap */
	FPackageIndex MapObject(const UObject* Object) const;

	// FArchive interface.
	using FArchiveUObject::operator<<; // For visibility of the overloads we don't override
	FArchive& operator<<( FName& InName );
	FArchive& operator<<( UObject*& Obj );
	FArchive& operator<<( FLazyObjectPtr& LazyObjectPtr );
	virtual void SetSerializeContext(FUObjectSerializeContext* InLoadContext) override;
	FUObjectSerializeContext* GetSerializeContext() override;
	virtual void UsingCustomVersion(const struct FGuid& Guid) override;
	/**
	 * Sets whether tagged property serialization should be replaced by faster unversioned serialization.
	 * This sets it on itself, the summary, the actual Saver Archive if any and set the proper associated flag on the LinkerRoot
	 */
	virtual void SetUseUnversionedPropertySerialization(bool bInUseUnversioned) override;


#if WITH_EDITOR
	// proxy for debugdata
	virtual void PushDebugDataString(const FName& DebugData) override { Saver->PushDebugDataString(DebugData); }
	virtual void PopDebugDataString() override { Saver->PopDebugDataString(); }
#endif

	virtual FString GetArchiveName() const override;

	/**
	 * If this archive is a FLinkerLoad or FLinkerSave, returns a pointer to the FLinker portion.
	 */
	virtual FLinker* GetLinker() { return this; }

	void Seek( int64 InPos );
	int64 Tell();
	// this fixes the warning : 'FLinkerSave::Serialize' hides overloaded virtual function
	using FLinker::Serialize;
	void Serialize( void* V, int64 Length );

	/** Invoke all of the callbacks in PostSaveCallbacks and then empty it. */
	void OnPostSave(const FPackagePath& PackagePath, FObjectPostSaveContext ObjectSaveContext);

	// FLinker interface
	virtual FString GetDebugName() const override;

	// LinkerSave interface
	/**
	 * Closes and deletes the Saver (file, memory or custom writer) which will close any associated file handle.
	 * Returns false if the owned saver contains errors after closing it, true otherwise.
	 */
	bool CloseAndDestroySaver();

	/**
	 * Sets a flag indicating that this archive contains data required to be gathered for localization.
	 */
	void ThisRequiresLocalizationGather();

	/** Get the filename being saved to */
	const FString& GetFilename() const;

	/* Set the output device used to log errors, if any. */
	void SetOutputDevice(FOutputDevice* InOutputDevice)
	{
		LogOutput = InOutputDevice;
	}

	/* Returns an output Device that can be used to log info, warnings and errors etc. */
	FOutputDevice* GetOutputDevice() const 
	{
		return LogOutput;
	}
protected:
	/** Set the filename being saved to */
	void SetFilename(FStringView InFilename);

private:
	/** Optional log output to bubble errors back up. */
	FOutputDevice* LogOutput = nullptr;
};
