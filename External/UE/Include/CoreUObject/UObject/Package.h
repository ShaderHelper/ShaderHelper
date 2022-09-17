// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "IO/PackageId.h"
#include "Misc/Guid.h"
#include "Misc/OutputDeviceError.h"
#include "Misc/ObjectThumbnail.h"
#include "Misc/PackagePath.h"
#include "Misc/SecureHash.h"
#include "Misc/WorldCompositionUtility.h"
#include "Serialization/CustomVersion.h"
#include "Templates/PimplPtr.h"
#include "Templates/UniquePtr.h"
#include "Async/Future.h"

class Error;

// This is a dummy type which is not implemented anywhere. It's only 
// used to flag a deprecated Conform argument to package save functions.
class FLinkerNull;
class FLinkerLoad;
class FLinkerSave;
class ITargetPlatform;
struct FPackageSaveInfo;
class FSavePackageContext;
struct FSavePackageArgs;

class UMetaData;

/**
* Represents the result of saving a package
*/
enum class ESavePackageResult
{
	/** Package was saved successfully */
	Success,
	/** Unknown error occured when saving package */
	Error,
	/** Canceled by user */
	Canceled,
	/** [When cooking] Package was not saved because it contained editor-only data */
	ContainsEditorOnlyData,
	/** [When cooking] Package was not saved because it was referenced by editor-only properties */
	ReferencedOnlyByEditorOnlyData,
	/** [When cooking] Package was not saved because it contains assets that were converted into native code */
	ReplaceCompletely,
	/** [When cooking] Package was saved, but we should generate a stub so that other converted packages can interface with it*/
	GenerateStub,
	DifferentContent	UE_DEPRECATED(5.0, "Diffing is now done using FDiffPackageWriter."),
	/** [When cooking] The file requested (when cooking on the fly) did not exist on disk */
	MissingFile,
	/** Result from ISavePackageValidator that indicates an error. */
	ValidatorError,
	/** Result from ISavePackageValidator that suppresses the save but is not an error. */
	ValidatorSuppress,
	/** Internal save result used to identify a valid empty internal save realm to skip over. @see ESaveRealm */
	EmptyRealm,
};

/**
* Struct returned from save package, contains the enum as well as extra data about what was written
*/
struct FSavePackageResultStruct
{
	/** Success/failure of the save operation */
	ESavePackageResult Result;

	/** Total size of all files written out, including bulk data */
	int64 TotalFileSize;

	/** MD5 hash of the cooked data */
	TFuture<FMD5Hash> CookedHash;

	/** Serialized package flags */
	uint32 SerializedPackageFlags;

	/** Linker for linker comparison after save. */
	TPimplPtr<FLinkerSave> LinkerSave;

	/** Constructors, it will implicitly construct from the result enum */
	FSavePackageResultStruct() : Result(ESavePackageResult::Error), TotalFileSize(0), SerializedPackageFlags(0) {}
	FSavePackageResultStruct(ESavePackageResult InResult) : Result(InResult), TotalFileSize(0), SerializedPackageFlags(0) {}
	FSavePackageResultStruct(ESavePackageResult InResult, int64 InTotalFileSize) : Result(InResult), TotalFileSize(InTotalFileSize), SerializedPackageFlags(0) {}
	FSavePackageResultStruct(ESavePackageResult InResult, int64 InTotalFileSize, TFuture<FMD5Hash>&& InHash, uint32 InSerializedPackageFlags, TPimplPtr<FLinkerSave> Linker = nullptr) : Result(InResult), TotalFileSize(InTotalFileSize), CookedHash(MoveTemp(InHash)), SerializedPackageFlags(InSerializedPackageFlags), LinkerSave(MoveTemp(Linker)) {}

	bool operator==(const FSavePackageResultStruct& Other) const
	{
		return Result == Other.Result;
	}

	bool operator!=(const FSavePackageResultStruct& Other) const
	{
		return Result != Other.Result;
	}

	/** Returns whether the package save was successful */
	bool IsSuccessful() const
	{
		return
			Result == ESavePackageResult::Success ||
			Result == ESavePackageResult::GenerateStub ||
			Result == ESavePackageResult::ReplaceCompletely;
	}
};

/**
* A package.
*/
PRAGMA_DISABLE_DEPRECATION_WARNINGS // Required for auto-generated functions referencing PackageFlags
class COREUOBJECT_API UPackage : public UObject
{
	DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR_NO_VTABLE_CTOR(UPackage, UObject, 0, TEXT("/Script/CoreUObject"), CASTCLASS_UPackage, NO_API)
	/** DO NOT USE. This constructor is for internal usage only for hot-reload purposes. */
	UPackage(FVTableHelper& Helper)
		: Super(Helper)
	{
	};

public:

	UPackage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{
	}

	/** delegate type for package dirty state events.  ( Params: UPackage* ModifiedPackage ) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPackageDirtyStateChanged, class UPackage*);
	/** delegate type for package saved events ( Params: const FString& PackageFileName, UObject* Outer ) */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPackageSaved, const FString&, UObject*);					
	/** delegate type for package saved events ( Params: const FString& PackageFileName, UObject* Outer, FObjectPostSaveContext ObjectSaveContext ) */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPackageSavedWithContext, const FString&, UPackage*, FObjectPostSaveContext);
	/** delegate type for when a package is marked as dirty via UObjectBaseUtilty::MarkPackageDirty ( Params: UPackage* ModifiedPackage, bool bWasDirty ) */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPackageMarkedDirty, class UPackage*, bool);
	/** delegate type for when a package is about to be saved */
	DECLARE_MULTICAST_DELEGATE_OneParam(FPreSavePackage, class UPackage*);
	/** delegate type for when a package is about to be saved */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FPreSavePackageWithContext, class UPackage*, FObjectPreSaveContext);

	UE_DEPRECATED(5.0, "Use PreSavePackageWithContextEvent instead.")
	static FPreSavePackage PreSavePackageEvent;
	/** Delegate to notify subscribers when a package is about to be saved. */
	static FPreSavePackageWithContext PreSavePackageWithContextEvent;
	UE_DEPRECATED(5.0, "Use PackageSavedWithContextEvent instead.")
	static FOnPackageSaved PackageSavedEvent;
	/** Delegate to notify subscribers when a package has been saved. This is triggered when the package saving
	*  has completed and was successful. */
	static FOnPackageSavedWithContext PackageSavedWithContextEvent;
	/** Delegate to notify subscribers when the dirty state of a package is changed.
	*  Allows the editor to register the modified package as one that should be prompted for source control checkout. 
	*  Use Package->IsDirty() to get the updated dirty state of the package */
	static FOnPackageDirtyStateChanged PackageDirtyStateChangedEvent;
	/** 
	* Delegate to notify subscribers when a package is marked as dirty via UObjectBaseUtilty::MarkPackageDirty 
	* Note: Unlike FOnPackageDirtyStateChanged, this is always called, even when the package is already dirty
	* Use bWasDirty to check the previous dirty state of the package
	* Use Package->IsDirty() to get the updated dirty state of the package
	*/
	static FOnPackageMarkedDirty PackageMarkedDirtyEvent;

private:
	/** Used by the editor to determine if a package has been changed.																							*/
	uint8	bDirty:1;

#if WITH_EDITORONLY_DATA
	/** True if this package is only referenced by editor-only properties */
	uint8 bLoadedByEditorPropertiesOnly:1;

	/** True if this package is a dynamic PIE package with external objects still loading */
	uint8 bIsDynamicPIEPackagePending:1;
#endif

public:

#if WITH_EDITORONLY_DATA
	/** True if this packages has been cooked for the editor / opened cooked by the editor */
	uint8 bIsCookedForEditor:1;
#endif

	/** Whether this package has been fully loaded (aka had all it's exports created) at some point.															*/
	mutable uint8 bHasBeenFullyLoaded:1;

#if WITH_EDITORONLY_DATA
	/** Set to true after serialization and postload and before returning from LoadPackage or calling load completion delegate, for loaded packages.			*/
private:
	uint8 bHasBeenEndLoaded : 1;
public:
	bool GetHasBeenEndLoaded() const
	{
		return bHasBeenEndLoaded != 0;
	}
	void SetHasBeenEndLoaded(bool bValue)
	{
		bHasBeenEndLoaded = bValue ? 1 : 0;
	}
#endif

	/**
	 * Whether this package can be imported, i.e. its package name is a package that exists on disk.
	 * Note: This includes all normal packages where the Name matches the FileName
	 * and localized packages shadowing an existing source package,
	 * but excludes level streaming packages with /Temp/ names.
	 */
	uint8 bCanBeImported:1;

private:
	// @note this should probably be entirely deprecated and removed but certain stat dump function are still using it, just compile it out in shipping for now
#if !UE_BUILD_SHIPPING
	/** Time in seconds it took to fully load this package. 0 if package is either in process of being loaded or has never been fully loaded. */
	float LoadTime;
#endif

#if WITH_EDITORONLY_DATA
	/** Indicates which folder to display this package under in the Generic Browser's list of packages. If not specified, package is added to the root level.	*/
	FName FolderName;

	/** Persistent GUID of package if it was loaded from disk. Persistent across saves. */
	FGuid PersistentGuid;

	/** Chunk IDs for the streaming install chunks this package will be placed in.  Empty for no chunk. Used during cooking. */
	TArray<int32> ChunkIDs;
#endif

#if !UE_STRIP_DEPRECATED_PROPERTIES
	/** GUID of package if it was loaded from disk. Changes at every save. */
	UE_DEPRECATED(4.27, "UPackage::Guid has not been used by the engine for a long time and it will be removed.")
	FGuid Guid;
#endif

	/** Package Flags */
	uint32	PackageFlagsPrivate;
	
	/** Globally unique id */
	FPackageId PackageId;

	/** The PackagePath this package was loaded from */
	FPackagePath LoadedPath;
public:

	/** Linker package version this package has been serialized with. This is mostly used by PostLoad **/
	UE_DEPRECATED(5.0, "Use Get/SetLinkerPackageVersion instead")
	FPackageFileVersion LinkerPackageVersion;

	/** Linker licensee version this package has been serialized with. This is mostly used by PostLoad **/
	UE_DEPRECATED(5.0, "Use Get/SetLinkerLicenseeVersion instead")
	int32 LinkerLicenseeVersion;

	/** Linker custom version container this package has been serialized with. This is mostly used by PostLoad **/
	UE_DEPRECATED(5.0, "Use Get/SetLinkerCustomVersions instead")
	FCustomVersionContainer LinkerCustomVersion;

	/** Linker load associated with this package */
	UE_DEPRECATED(5.0, "Use Get/SetLinker instead")
	FLinkerLoad* LinkerLoad;

	/** size of the file for this package; if the package was not loaded from a file or was a forced export in another package, this will be zero */
	UE_DEPRECATED(5.0, "Use Get/SetFileSize instead")
	uint64 FileSize;

#if WITH_EDITORONLY_DATA
	/** Editor only: Thumbnails stored in this package */
	UE_DEPRECATED(5.0, "Use Get/SetThumbnailMap instead")
	TUniquePtr< FThumbnailMap > ThumbnailMap;

	// MetaData for the editor, or NULL in the game
	UE_DEPRECATED(5.0, "Use Get/HasMetaData instead")
	class UMetaData* MetaData;

	/** Editor only: PIE instance ID this package belongs to, INDEX_NONE otherwise */
	UE_DEPRECATED(5.0, "Use Get/SetPIEInstanceID instead")
	int32 PIEInstanceID;
#endif

#if !UE_STRIP_DEPRECATED_PROPERTIES
	/** The name of the file that this package was loaded from */
	UE_DEPRECATED(5.0, "Use GetLoadedPath instead")
	FName FileName;
#endif

	// World browser information
	UE_DEPRECATED(5.0, "Use Get/SetWorldTileInfo instead")
	TUniquePtr< FWorldTileInfo > WorldTileInfo;

#if WITH_RELOAD
	/** Link list of delegates registered to the package.  The next pointer chain can't be used for this. */
	UE_DEPRECATED(5.0, "Use Get/SetReloadDelegates instead")
	TArray<UFunction*> Delegates;
#endif

public:

	virtual bool IsNameStableForNetworking() const override { return true; }		// For now, assume all packages have stable net names
	virtual bool NeedsLoadForClient() const override { return true; }				// To avoid calling the expensive generic version, which only makes sure that the UPackage static class isn't excluded
	virtual bool NeedsLoadForServer() const override { return true; }
	virtual bool IsPostLoadThreadSafe() const override;
	virtual bool IsDestructionThreadSafe() const override { return true; }

#if WITH_EDITORONLY_DATA
	/** Sets the bLoadedByEditorPropertiesOnly flag */
	void SetLoadedByEditorPropertiesOnly(bool bIsEditorOnly, bool bRecursive = false);
	/** returns true when the package is only referenced by editor-only flag */
	bool IsLoadedByEditorPropertiesOnly() const { return bLoadedByEditorPropertiesOnly; }

	/** Sets the bIsDynamicPIEPackagePending flag */
	void SetDynamicPIEPackagePending(bool bInIsDynamicPIEPackagePending) { bIsDynamicPIEPackagePending = bInIsDynamicPIEPackagePending; }
	/** returns the bIsDynamicPIEPackagePending flag */
	bool IsDynamicPIEPackagePending() const { return bIsDynamicPIEPackagePending; }
#endif

	/**
	* Called after the C++ constructor and after the properties have been initialized, but before the config has been loaded, etc.
	* mainly this is to emulate some behavior of when the constructor was called after the properties were initialized.
	*/
	virtual void PostInitProperties() override;

	virtual void FinishDestroy() override;

	/** Serializer */
	virtual void Serialize( FArchive& Ar ) override;

	/** Packages are never assets */
	virtual bool IsAsset() const override { return false; }

	// UPackage interface.

private:
	friend class FLinkerLoad;
	friend class FUnsafeLinkerLoad;
	friend class FSaveContext;
	friend struct FAsyncPackage2;

	void SetLinker(FLinkerLoad* InLinker)
	{
		LinkerLoad = InLinker;
	}

	void SetLinkerPackageVersion(FPackageFileVersion InVersion)
	{
		LinkerPackageVersion = MoveTemp(InVersion);
	}

	void SetLinkerLicenseeVersion(int32 InVersion)
	{
		LinkerLicenseeVersion = InVersion;
	}

	void SetLinkerCustomVersions(FCustomVersionContainer InVersions)
	{
		LinkerCustomVersion = MoveTemp(InVersions);
	}

	void SetFileSize(int64 InFileSize)
	{
		FileSize = InFileSize;
	}

	void SetMetaData(UMetaData* InMetaData)
	{
#if WITH_EDITORONLY_DATA
		MetaData = InMetaData;
#endif
	}

public:

	/**
	 * Returns the PIE instance id used by the package if any, or INDEX_NONE otherwise 
	 */
	int32 GetPIEInstanceID() const
	{
#if WITH_EDITORONLY_DATA
		return PIEInstanceID;
#else
		return INDEX_NONE;
#endif
	}

	/**
	 * Set the PIE instance id for this package
	 * @param InPIEInstanceID The PIE instance id to use or INDEX_NONE to remove any id set
	 */
	void SetPIEInstanceID(int32 InPIEInstanceID)
	{
#if WITH_EDITORONLY_DATA
		PIEInstanceID = InPIEInstanceID;
#endif
	}

	FLinkerLoad* GetLinker() const
	{
		return LinkerLoad;
	}

	const FPackageFileVersion& GetLinkerPackageVersion() const
	{
		return LinkerPackageVersion;
	}

	int32 GetLinkerLicenseeVersion() const
	{
		return LinkerLicenseeVersion;
	}

	const FCustomVersionContainer& GetLinkerCustomVersions() const
	{
		return LinkerCustomVersion;
	}


	/**
	 * Sets the time it took to load this package.
	 */
	void SetLoadTime( float InLoadTime ) 
	{
#if !UE_BUILD_SHIPPING
		LoadTime = InLoadTime;
#endif
	}

	/**
	* Returns the time it took the last time this package was fully loaded, 0 otherwise.
	*
	* @return Time it took to load.
	*/
	float GetLoadTime() const
	{
#if !UE_BUILD_SHIPPING
		return LoadTime;
#else
		return 0.0f;
#endif
	}

#if WITH_EDITORONLY_DATA
	/**
	* Get the package's folder name
	* @return		Folder name
	*/
	FName GetFolderName() const
	{
		return FolderName;
	}

	/**
	* Set the package's folder name
	*/
	void SetFolderName (FName name)
	{
		FolderName = name;
	}
#endif

	/**
	 * Clear the package dirty flag without any transaction tracking
	 */
	void ClearDirtyFlag()
	{
		bDirty = false;
	}

	/**
	* Marks/Unmarks the package's bDirty flag, save the package to the transaction buffer if a transaction is ongoing
	*/
	void SetDirtyFlag( bool bIsDirty );

	/**
	* Returns whether the package needs to be saved.
	*
	* @return		true if the package is dirty and needs to be saved, false otherwise.
	*/
	bool IsDirty() const
	{
		return bDirty;
	}

	/**
	* Marks this package as being fully loaded.
	*/
	void MarkAsFullyLoaded()
	{
		bHasBeenFullyLoaded = true;
	}

	/**
	* Returns whether the package is fully loaded.
	*
	* @return true if fully loaded or no file associated on disk, false otherwise
	*/
	bool IsFullyLoaded() const;

	/**
	* Fully loads this package. Safe to call multiple times and won't clobber already loaded assets.
	*/
	void FullyLoad();

	/**
	 * Get the path this package was loaded from; may be different than packagename, and may not be set if the package was not loaded from disk
	 */
	const FPackagePath& GetLoadedPath() const;

	/**
	 * Set the path this package was loaded from; typically called only by the linker during load
	 */
	void SetLoadedPath(const FPackagePath& PackagePath);

	/**
	* Marks/Unmarks the package's bCanBeImported flag.
	*/
	void SetCanBeImportedFlag(bool bInCanBeImported)
	{
		bCanBeImported = bInCanBeImported;
	}

	/**
	* Returns whether the package can be imported.
	*
	* @return		true if the package can be imported.
	*/
	bool CanBeImported() const
	{
		return bCanBeImported;
	}
	/**
	* Tags the Package's metadata
	*/
	virtual void TagSubobjects(EObjectFlags NewFlags) override;

	/**
	* Called to indicate that this package contains a ULevel or UWorld object.
	*/
	void ThisContainsMap() 
	{
		SetPackageFlags(PKG_ContainsMap);
	}

	/**
	* Returns whether this package contains a ULevel or UWorld object.
	*
	* @return		true if package contains ULevel/ UWorld object, false otherwise.
	*/
	bool ContainsMap() const
	{
		return HasAnyPackageFlags(PKG_ContainsMap);
	}

	/**
	* Called to indicate that this package contains data required to be gathered for localization.
	*/
	void ThisRequiresLocalizationGather(bool Value)
	{
		if(Value)
		{
			SetPackageFlags(PKG_RequiresLocalizationGather);
		}
		else
		{
			ClearPackageFlags(PKG_RequiresLocalizationGather);
		}
	}

	/**
	* Returns whether this package contains data required to be gathered for localization.
	*
	* @return		true if package contains contains data required to be gathered for localization, false otherwise.
	*/
	bool RequiresLocalizationGather() const
	{
		return HasAnyPackageFlags(PKG_RequiresLocalizationGather);
	}

	// UE-21181 - trying to track when a flag gets set on a package due to PIE
#if WITH_EDITOR
	static UPackage* EditorPackage;
	void SetPackageFlagsTo( uint32 NewFlags );
#else
	/**
	* Sets all package flags to the specified values.
	*
	* @param	NewFlags		New value for package flags
	*/
	FORCEINLINE void SetPackageFlagsTo( uint32 NewFlags )
	{
		PackageFlagsPrivate = NewFlags;
	}
#endif

	/**
	* Set the specified flags to true. Does not affect any other flags.
	*
	* @param	NewFlags		Package flags to enable
	*/
	FORCEINLINE void SetPackageFlags( uint32 NewFlags )
	{
		SetPackageFlagsTo(PackageFlagsPrivate | NewFlags);
	}

	/**
	* Set the specified flags to false. Does not affect any other flags.
	*
	* @param	NewFlags		Package flags to disable
	*/
	FORCEINLINE void ClearPackageFlags( uint32 NewFlags )
	{
		SetPackageFlagsTo(PackageFlagsPrivate & ~NewFlags);
	}

	/**
	* Used to safely check whether the passed in flag is set.
	*
	* @param	FlagsToCheck		Package flags to check for
	*
	* @return	true if the passed in flag is set, false otherwise
	*			(including no flag passed in, unless the FlagsToCheck is CLASS_AllFlags)
	*/
	FORCEINLINE bool HasAnyPackageFlags( uint32 FlagsToCheck ) const
	{
		return (PackageFlagsPrivate & FlagsToCheck) != 0;
	}

	/**
	* Used to safely check whether all of the passed in flags are set.
	*
	* @param FlagsToCheck	Package flags to check for
	* @return true if all of the passed in flags are set (including no flags passed in), false otherwise
	*/
	FORCEINLINE bool HasAllPackagesFlags( uint32 FlagsToCheck ) const
	{
		return ((PackageFlagsPrivate & FlagsToCheck) == FlagsToCheck);
	}

	/**
	* Gets the package flags.
	*
	* @return	The package flags.
	*/
	FORCEINLINE uint32 GetPackageFlags() const
	{
		return PackageFlagsPrivate;
	}

#if WITH_EDITORONLY_DATA
	/** Returns true if this package has a thumbnail map */
	bool HasThumbnailMap() const
	{
		return ThumbnailMap.IsValid();
	}

	/** Returns the thumbnail map for this package (const).  Only call this if HasThumbnailMap returns true! */
	const FThumbnailMap& GetThumbnailMap() const
	{
		check( HasThumbnailMap() );
		return *ThumbnailMap;
	}

	/** Access the thumbnail map for this package.  Only call this if HasThumbnailMap returns true! */
	FThumbnailMap& AccessThumbnailMap()
	{
		check( HasThumbnailMap() );
		return *ThumbnailMap;
	}

	/** Set the internal thumbnail map for this package. */
	void SetThumbnailMap(TUniquePtr<FThumbnailMap> InThumbnailMap)
	{
		ThumbnailMap = MoveTemp(InThumbnailMap);
	}

	/** returns our persistent Guid */
	FORCEINLINE FGuid GetPersistentGuid() const
	{
		return PersistentGuid;
	}
	/** sets a specific persistent Guid */
	FORCEINLINE void SetPersistentGuid(FGuid NewPersistentGuid)
	{
		PersistentGuid = NewPersistentGuid;
	}
#endif

#if WITH_RELOAD
	const TArray<UFunction*>& GetReloadDelegates() const
	{
		return Delegates;
	}

	void SetReloadDelegates(TArray<UFunction*> InDelegates) 
	{
		Delegates = MoveTemp(InDelegates);
	}
#endif


	/** Get the world tile info if any*/
	FWorldTileInfo* GetWorldTileInfo() const
	{
		return WorldTileInfo.Get();
	}

	/** Set the world tile info */
	void SetWorldTileInfo(TUniquePtr<FWorldTileInfo> InWorldTileInfo)
	{
		WorldTileInfo = MoveTemp(InWorldTileInfo);
	}

	/** returns our Guid */
	UE_DEPRECATED(4.27, "UPackage::Guid has not been used by the engine for a long time and GetGuid will be removed.")
	FORCEINLINE FGuid GetGuid() const
	{
#if !UE_STRIP_DEPRECATED_PROPERTIES
		return Guid;
#else
		return FGuid();
#endif
	}
	/** makes our a new fresh Guid */
	UE_DEPRECATED(4.27, "UPackage::Guid has not been used by the engine for a long time and MakeNewGuid will be removed.")
	FORCEINLINE FGuid MakeNewGuid()
	{
#if !UE_STRIP_DEPRECATED_PROPERTIES
		Guid = FGuid::NewGuid();
		return Guid;
#else
		return FGuid();
#endif
	}
	/** sets a specific Guid */
	UE_DEPRECATED(4.27, "UPackage::Guid has not been used by the engine for a long time and SetGuid will be removed.")
	FORCEINLINE void SetGuid(FGuid NewGuid)
	{
#if !UE_STRIP_DEPRECATED_PROPERTIES
		Guid = NewGuid;
#endif
	}

	/** returns our FileSize */
	FORCEINLINE int64 GetFileSize() const
	{
		return FileSize;
	}

	/** returns our ChunkIDs */
	const TArray<int32>& GetChunkIDs() const
	{
#if WITH_EDITORONLY_DATA
		return ChunkIDs;
#else
		static TArray<int32> Dummy;
		return Dummy;
#endif
	}

	/** sets our ChunkIDs */
	FORCEINLINE void SetChunkIDs(const TArray<int32>& InChunkIDs)
	{
#if WITH_EDITORONLY_DATA
		ChunkIDs = InChunkIDs;
#endif
	}

	/** returns the unique package id */
	FORCEINLINE FPackageId GetPackageId() const
	{
		return PackageId;
	}

	/** sets the unique package id */
	FORCEINLINE void SetPackageId(FPackageId InPackageId)
	{
		PackageId = InPackageId;
	}

	/** returns the unique package id to load */
	FORCEINLINE FPackageId GetPackageIdToLoad() const
	{
		return FPackageId::FromName(LoadedPath.GetPackageFName());
	}

	/**
	 * Utility function to find Asset in this package, if any
	 * @return the asset in the package, if any
	 */
	UObject* FindAssetInPackage(EObjectFlags RequiredTopLevelFlags = RF_NoFlags) const;

	/**
	 * Return the list of packages found assigned to object outer-ed to the top level objects of this package
	 * @return the array of external packages
	 */
	TArray<UPackage*> GetExternalPackages() const;

	////////////////////////////////////////////////////////
	// MetaData 

	/** 
	 * Return if the package currently has an assigned metadata object
	 * @returns true if there's an assigned metadata
	 */
	bool HasMetaData() const
	{
#if WITH_EDITORONLY_DATA
		return MetaData != nullptr;
#else
		return false;
#endif
	}

	/**
	* Gets (after possibly creating) a metadata object for this package
	*
	* @param	bAllowLoadObject				Can load an object to find it's UMetaData if not currently loaded.
	*
	* @return A valid UMetaData pointer for all objects in this package
	*/
	class UMetaData* GetMetaData();

	/**
	* Save one specific object (along with any objects it references contained within the same Outer) into an Unreal package.
	* 
	* @param	InOuter			the outer to use for the new package
	* @param	InAsset			the object that should be saved into the package
	* @param	Filename		the name to use for the new package file
	* @param	SaveArgs		Extended arguments to control the save
	* @see		FSavePackageContext
	*
	* @return	FSavePackageResultStruct enum value with the result of saving a package as well as extra data
	*/
	static FSavePackageResultStruct Save(UPackage* InOuter, UObject* InAsset, const TCHAR* Filename, const FSavePackageArgs& SaveArgs);

	UE_DEPRECATED(5.0, "Pack the arguments into FSavePackageArgs and call the function overload that takes FSavePackageArgs. Note that Conform and InOutDiffMap are no longer implemented.")
	static FSavePackageResultStruct Save(UPackage* InOuter, UObject* Base, EObjectFlags TopLevelFlags,
		const TCHAR* Filename, FOutputDevice* Error = GError, FLinkerNull* Conform = nullptr,
		bool bForceByteSwapping = false, bool bWarnOfLongFilename = true, uint32 SaveFlags = SAVE_None,
		const ITargetPlatform* TargetPlatform = nullptr, const FDateTime& FinalTimeStamp = FDateTime::MinValue(),
		bool bSlowTask = true, class FArchiveDiffMap* InOutDiffMap = nullptr,
		FSavePackageContext* SavePackageContext = nullptr);

	/**
	 * Save a list of packages concurrently using Save2 mechanism
	 * SaveConcurrent is currently experimental and shouldn't be used until it can safely replace Save.
	 */
	static ESavePackageResult SaveConcurrent(TArrayView<FPackageSaveInfo> InPackages, const FSavePackageArgs& SaveArgs, TArray<FSavePackageResultStruct>& OutResults);

	/**
	* Save one specific object (along with any objects it references contained within the same Outer) into an Unreal package.
	*
	* @param	InOuter			the outer to use for the new package
	* @param	InAsset			the object that should be saved into the package
	* @param	Filename		the name to use for the new package file
	* @param	SaveArgs		Extended arguments to control the save
	* @see		FSavePackageContext
	*
	* @return	true if the package was saved successfully.
	*/
	static bool SavePackage(UPackage* InOuter, UObject* InAsset, const TCHAR* Filename, const FSavePackageArgs& SaveArgs);

	UE_DEPRECATED(5.0, "Pack the arguments into FSavePackageArgs and call the function overload that takes FSavePackageArgs. Note that Conform is no longer implemented.")
	static bool SavePackage(UPackage* InOuter, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* Filename,
		FOutputDevice* Error = GError, FLinkerNull* Conform = nullptr, bool bForceByteSwapping = false,
		bool bWarnOfLongFilename = true, uint32 SaveFlags = SAVE_None, const ITargetPlatform* TargetPlatform = nullptr,
		const FDateTime& FinalTimeStamp = FDateTime::MinValue(), bool bSlowTask = true);

	/** Wait for any SAVE_Async file writes to complete **/
	static void WaitForAsyncFileWrites();

	/**
	* Determines if a package contains no more assets.
	*
	* @param Package			the package to test
	* @param LastReferencer	the optional last UObject referencer to this package. This object will be excluded when determining if the package is empty
	* @return true if Package contains no more assets.
	*/
	static bool IsEmptyPackage(UPackage* Package, const UObject* LastReferencer = NULL);

private:
	static FSavePackageResultStruct Save2(UPackage* InPackage, UObject* InAsset, const TCHAR* InFilename, const FSavePackageArgs& SaveArgs);
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS