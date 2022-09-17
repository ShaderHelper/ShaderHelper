// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RedirectCollector:  Editor-only global object that handles resolving redirectors and handling string asset cooking rules
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"

#if WITH_EDITOR

enum class ESoftObjectPathCollectType : uint8;

class COREUOBJECT_API FRedirectCollector
{
private:
	
	/** Helper struct for soft object path tracking */
	struct FSoftObjectPathProperty
	{
		FSoftObjectPathProperty(FName InAssetPathName, FName InProperty, bool bInReferencedByEditorOnlyProperty)
			: AssetPathName(InAssetPathName)
			, PropertyName(InProperty)
			, bReferencedByEditorOnlyProperty(bInReferencedByEditorOnlyProperty)
		{}

		 bool operator==(const FSoftObjectPathProperty& Other) const
		 {
		 	return AssetPathName == Other.AssetPathName &&
		 		PropertyName == Other.PropertyName &&
		 		bReferencedByEditorOnlyProperty == Other.bReferencedByEditorOnlyProperty;
		 }

		friend inline uint32 GetTypeHash(const FSoftObjectPathProperty& Key)
		{
			uint32 Hash = 0;
			Hash = HashCombine(Hash, GetTypeHash(Key.AssetPathName));
			Hash = HashCombine(Hash, GetTypeHash(Key.PropertyName));
			Hash = HashCombine(Hash, (uint32)Key.bReferencedByEditorOnlyProperty);
			return Hash;
		}

		const FName& GetAssetPathName() const
		{
			return AssetPathName;
		}

		const FName& GetPropertyName() const
		{
			return PropertyName;
		}

		bool GetReferencedByEditorOnlyProperty() const
		{
			return bReferencedByEditorOnlyProperty;
		}

	private:
		FName AssetPathName;
		FName PropertyName;
		bool bReferencedByEditorOnlyProperty;
	};

public:

	/**
	 * Called from FSoftObjectPath::PostLoadPath, registers the given SoftObjectPath for later querying
	 * @param InPath The soft object path that was loaded
	 * @Param InArchive The archive that loaded this path
	 */
	void OnSoftObjectPathLoaded(const struct FSoftObjectPath& InPath, FArchive* InArchive);

	/**
	 * Called at the end of Package Save to record soft package references that might have been created by save transformations
	 * @param ReferencingPackage The package on which we are recording the references
	 * @param PackageNames List of of soft package references needed by the referencing package
	 * @param bEditorOnlyReferences if the PackageNames list are references made by editor only properties
	 */
	void CollectSavedSoftPackageReferences(FName ReferencingPackage, const TSet<FName>& PackageNames, bool bEditorOnlyReferences);

	/**
	 * Load all soft object paths to resolve them, add that to the remap table, and empty the array
	 * @param FilterPackage If set, only load references that were created by FilterPackage. If empty, resolve  all of them
	 */
	void ResolveAllSoftObjectPaths(FName FilterPackage = NAME_None);

	/**
	 * Returns the list of packages referenced by soft object paths loaded by FilterPackage, and remove them from the internal list
	 * @param FilterPackage Return references made by loading this package. If passed null will return all references made with no explicit package
	 * @param bGetEditorOnly If true will return references loaded by editor only objects, if false it will not
	 * @param OutReferencedPackages Return list of packages referenced by FilterPackage
	 */
	void ProcessSoftObjectPathPackageList(FName FilterPackage, bool bGetEditorOnly, TSet<FName>& OutReferencedPackages);

	/** Adds a new mapping for redirector path to destination path, this is called from the Asset Registry to register all redirects it knows about */
	void AddAssetPathRedirection(FName OriginalPath, FName RedirectedPath);

	/** Removes an asset path redirection, call this when deleting redirectors */
	void RemoveAssetPathRedirection(FName OriginalPath);

	/** Returns a remapped asset path, if it returns null there is no relevant redirector */
	FName GetAssetPathRedirection(FName OriginalPath);

	/**
	 * Do we have any references to resolve.
	 * @return true if we have references to resolve
	 */
	bool HasAnySoftObjectPathsToResolve() const
	{
		return SoftObjectPathMap.Num() > 0;
	}

	/**
	 * Removes and copies the value of the list of package dependencies of the given package that were
	 * marked as excluded by FSoftObjectPathSerializationScopes during the load of the package.
	 * This is only used on startup packages during the cook commandlet; for all other packages and
	 * modes it will find an empty list and return false.
	 * @param OutExcludedReferences Out set that is reset and then appended with any discovered values
	 * @return Whether any references were found
	 */
	bool RemoveAndCopySoftObjectPathExclusions(FName PackageName, TSet<FName>& OutExcludedReferences);

	/** Called from the cooker to stop the tracking of exclusions. */
	void OnStartupPackageLoadComplete();

private:

	/** A map of assets referenced by soft object paths, with the key being the package with the reference */
	typedef TSet<FSoftObjectPathProperty> FSoftObjectPathPropertySet;
	typedef TMap<FName, FSoftObjectPathPropertySet> FSoftObjectPathMap;

	/** Return whether SoftObjectPathExclusions are currently being tracked, based on commandline and cook phase. */
	bool ShouldTrackPackageReferenceTypes();

	/** The discovered references that should be followed during cook */
	FSoftObjectPathMap SoftObjectPathMap;
	/** The discovered references to packages and the collect type for whether they should be followed during cook. */
	TMap<FName, TMap<FName, ESoftObjectPathCollectType>> PackageReferenceTypes;

	/** When saving, apply this remapping to all soft object paths */
	TMap<FName, FName> AssetPathRedirectionMap;

	/** For SoftObjectPackageMap map */
	FCriticalSection CriticalSection;

	enum class ETrackingReferenceTypesState : uint8
	{
		Uninitialized,
		Disabled,
		Enabled,
	};
	ETrackingReferenceTypesState TrackingReferenceTypesState;
};

// global redirect collector callback structure
COREUOBJECT_API extern FRedirectCollector GRedirectCollector;

#endif // WITH_EDITOR
