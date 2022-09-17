// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/ContainersFwd.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

namespace EAssetRegistryDependencyType
{
	enum Type
	{
		// Dependencies which don't need to be loaded for the object to be used (i.e. soft object paths)
		Soft = 0x01,

		// Dependencies which are required for correct usage of the source asset, and must be loaded at the same time
		Hard = 0x02,

		// References to specific SearchableNames inside a package
		SearchableName = 0x04,

		// Indirect management references, these are set through recursion for Primary Assets that manage packages or other primary assets
		SoftManage = 0x08,

		// Reference that says one object directly manages another object, set when Primary Assets manage things explicitly
		HardManage = 0x10,

		// Note: Also update FAssetRegistryDependencyOptions when adding more flags
	};

	static const Type None = (Type)(0);
	static const Type All = (Type)(Soft | Hard | SearchableName | SoftManage | HardManage);
	static const Type Packages = (Type)(Soft | Hard);
	static const Type Manage = (Type)(SoftManage | HardManage);
}

class IAssetRegistry;
class UAssetRegistryImpl;

namespace UE::AssetRegistry
{

	/**
	 * Multiple meanings of dependency are used in the AssetRegistry; the category specifes which meaning is referred to.
	 * During queries for dependencies, the Category variable can be a bitfield combination of Category values, in which case dependencies in any of the specified categories are returned.
	 */
	// 
	enum class EDependencyCategory : uint8
	{
		Package = 0x01,			// The target asset of any package dependency is expected to be loadable whenever the source asset is available; see EDependencyProperty for different requirements of the loadability.
		Manage = 0x02,			// The target asset of any manage dependency is managed (e.g. given a disk layout location) either directly or indirectly by the source asset. Used by UAssetManager.
		SearchableName = 0x04,  // Targets of SearchableName dependencies are FNames Keys inside of an Asset. The Source Asset of the dependency has a value for that Key. Used to search for Assets with a given Key,Value for the custom Key.

		None = 0x0,
		All = Package | Manage | SearchableName,
	};
	ENUM_CLASS_FLAGS(EDependencyCategory);

	/**
	 * Properties that might be possessed by a dependency. Each property is specific to a EDependencyCategory value.
	 */
	enum class EDependencyProperty : uint8
	{
		None = 0,

		// Package Dependencies
		PackageMask = 0x7,
		Hard = 0x1,			// The target asset must be loaded before the source asset can finish loading. The lack of this property is known as a Soft dependency, and indicates only that the source asset expects the target asset to be loadable on demand. 
		Game = 0x2,			// The target asset is needed in the game as well as the editor. The lack of this property is known as an EditorOnly dependency.
		Build = 0x4,		// Fields on the target asset are used in the transformation of the source asset during cooking in addition to being required in the game or editor. The lack of this property indicates that the target asset is required in game or editor, but is not required during cooking.

		// SearchableName Dependencies
		SearchableNameMask = 0x0, // None yet

		// ManageDependencies
		ManageMask = 0x8,
		Direct = 0x8,		// The target asset was specified explicitly as a managee by the source asset. Lack of this property is known as an indirect dependency; the target asset is reachable by following the transitive closure of Direct Manage Dependencies and Package dependencies from the source asset.

		AllMask = PackageMask | SearchableNameMask | ManageMask,
	};
	ENUM_CLASS_FLAGS(EDependencyProperty);

	/**
	 * Flags that specify required properties (or required-not-present properties) for a dependency to be returned from a query.
	 * Values in this enum correspond to values in EDependencyProperty; each EDependencyProperty value has a positive and negative equivalent in this enum.
	 * This allows a single bitfield to indicate required-present, required-not-present, or dont-care for each property.
	 * For any category-specific values, those values apply only to dependencies in the category, and do not impose restrictions on dependencies from other categories.
	 */
	enum class EDependencyQuery : uint32
	{
		NoRequirements = 0,

		// Package Dependencies Only
		Hard = 0x0001,			// Return only dependencies with EDependencyProperty::Hard
		NotHard = 0x002,		// Return only dependencies without EDependencyProperty::Hard
		Soft = NotHard,

		Game = 0x004,			// Return only dependencies with EDependencyProperty::Game
		NotGame = 0x008,		// Return only dependencies without EDependencyProperty::Game
		EditorOnly = NotGame,

		Build = 0x010,			// Return only dependencies with EDependencyProperty::Build
		NotBuild = 0x020,		// Return only dependencies without EDependencyProperty::Build

		// Manage Dependencies Only
		Direct = 0x0400,		// Return only dependencies with EDependencyProperty::Direct
		NotDirect = 0x0800,		// Return only dependencies without EDependencyProperty::Direct
		Indirect = NotDirect,

		// Masks used for manipulating EDependencyQuerys
		PackageMask = 0x00ff,
		SearchableNameMask = 0x0000, // None yet
		ManageMask = 0x0f00,
	};
	ENUM_CLASS_FLAGS(EDependencyQuery);

	/** Options used to read/write the DevelopmentAssetRegistry when serializing, which includes all data */
	enum class ESerializationTarget : uint8
	{
		ForGame,
		ForDevelopment
	};

	/**
	 * A struct that is equivalent to EDependencyQuery, but is more useful for performance in filtering operations.
	 * This is used by the filter implementations inside of GetDependency/GetReferencer calls; callers of those functions can instead use the more convenient values in EDependencyQuery.
	 */
	struct FDependencyQuery
	{
		UE::AssetRegistry::EDependencyProperty Required; // Only Dependencies that possess all of these properties will be returned. Note that flags specific to another EDependencyCategory are ignored when querying dependencies in a given category.
		UE::AssetRegistry::EDependencyProperty Excluded; // Only Dependencies that possess none of these properties will be returned. Note that flags specific to another EDependencyCategory are ignored when querying dependencies in a given category.

		FDependencyQuery()
		{
			Required = UE::AssetRegistry::EDependencyProperty::None;
			Excluded = UE::AssetRegistry::EDependencyProperty::None;
		}

		inline FDependencyQuery(EDependencyQuery QueryFlags)
		{
			Required = (!!(QueryFlags & EDependencyQuery::Hard) ? UE::AssetRegistry::EDependencyProperty::Hard : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::Game) ? UE::AssetRegistry::EDependencyProperty::Game : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::Build) ? UE::AssetRegistry::EDependencyProperty::Build : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::Direct) ? UE::AssetRegistry::EDependencyProperty::Direct : UE::AssetRegistry::EDependencyProperty::None);
			Excluded = (!!(QueryFlags & EDependencyQuery::NotHard) ? UE::AssetRegistry::EDependencyProperty::Hard : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::NotGame) ? UE::AssetRegistry::EDependencyProperty::Game : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::NotBuild) ? UE::AssetRegistry::EDependencyProperty::Build : UE::AssetRegistry::EDependencyProperty::None)
				| (!!(QueryFlags & EDependencyQuery::NotDirect) ? UE::AssetRegistry::EDependencyProperty::Direct : UE::AssetRegistry::EDependencyProperty::None);
		}

		UE_DEPRECATED(4.26, "Helper function for backwards compatibility")
		inline explicit FDependencyQuery(EAssetRegistryDependencyType::Type DependencyType)
		{
			Required = (DependencyType & EAssetRegistryDependencyType::Soft) ? UE::AssetRegistry::EDependencyProperty::None : UE::AssetRegistry::EDependencyProperty::Hard;
			Excluded = (DependencyType & EAssetRegistryDependencyType::Hard) ? UE::AssetRegistry::EDependencyProperty::None : UE::AssetRegistry::EDependencyProperty::Hard;
			Required |= (DependencyType & EAssetRegistryDependencyType::SoftManage) ? UE::AssetRegistry::EDependencyProperty::None : UE::AssetRegistry::EDependencyProperty::Direct;
			Excluded |= (DependencyType & EAssetRegistryDependencyType::HardManage) ? UE::AssetRegistry::EDependencyProperty::None : UE::AssetRegistry::EDependencyProperty::Direct;
		}

		FDependencyQuery(const FDependencyQuery& Other) = default;
		FDependencyQuery& operator=(const FDependencyQuery& Other) = default;
	};

	// Functions to read and write the data used by the AssetRegistry in each package; the format of this data is separate from the format of the data in the asset registry
	COREUOBJECT_API void WritePackageData(FStructuredArchiveRecord& ParentRecord, bool bIsCooking, const UPackage* Package, FLinkerSave* Linker, const TSet<UObject*>& ImportsUsedInGame, const TSet<FName>& SoftPackagesUsedInGame);
	// ReadPackageDataMain and ReadPackageDataDependencies are declared in IAssetRegistry.h, in the AssetRegistry module, because they depend upon some structures defined in the AssetRegistry module

	namespace Private
	{
		/**
		 * Storage for the singleton IAssetRegistry*
		 * TODO: this storage should be a class static variable on IAssetRegistry, but that type is defined in the AssetRegistry module, and many modules try to access the singleton (and call virtual functions on it) without linking against
		 * the AssetRegistry module, so the storage for the singleton needs to be defined in a lower-level module that all of those modules do include
		 */
		class COREUOBJECT_API IAssetRegistrySingleton
		{
		public:
			static IAssetRegistry* Get()
			{
				return Singleton;
			}
		private:
			static IAssetRegistry* Singleton;
			friend class ::UAssetRegistryImpl;
		};
	}

	class COREUOBJECT_API FFiltering
	{
	public:
		/** Return whether to filter out assets of the given class and flags from the editor's asset registry */
		static bool ShouldSkipAsset(FName AssetClass, uint32 PackageFlags);

		/** Return whether to filter out the given object (assumed to be an asset) from the editor's asset registry */
		static bool ShouldSkipAsset(const UObject* InAsset);

		/** Call to invalidate the list of skip assets and cause their next use to recreate them on demand */
		static void MarkDirty();

#if WITH_ENGINE && WITH_EDITOR
		/** Copy the global skip classes set from the given external sets that were already populated. */
		static void SetSkipClasses(const TSet<FName>& InSkipUncookedClasses, const TSet<FName>& InSkipCookedClasses);
#endif
	};

#if WITH_ENGINE && WITH_EDITOR
namespace Utils
{
	/** Return whether to filter out assets of the given class and flags based on the skip classes */
	COREUOBJECT_API bool ShouldSkipAsset(FName AssetClass, uint32 PackageFlags,
		const TSet<FName>& InSkipUncookedClasses, const TSet<FName>& InSkipCookedClasses);
	/** Return whether to filter out the given object (assumed to be an asset) based on the skip classes */
	COREUOBJECT_API bool ShouldSkipAsset(const UObject* InAsset,
		const TSet<FName>& InSkipUncookedClasses, const TSet<FName>& InSkipCookedClasses);
	/** Run the calculation of which classes to skip and store results in the given sets. */
	COREUOBJECT_API void PopulateSkipClasses(TSet<FName>& OutSkipUncookedClasses, TSet<FName>& OutSkipCookedClasses);
}
#endif

}

// Enums used in public Engine headers
namespace EAssetSetManagerResult
{
	enum Type
	{
		DoNotSet,			// Do not set manager
		SetButDoNotRecurse,	// Set but do not recurse
		SetAndRecurse		// Set and recurse into reference
	};
}

namespace EAssetSetManagerFlags
{
	enum Type
	{
		IsDirectSet = 1,				// This attempt is a direct set instead of a recursive set
		TargetHasExistingManager = 2,	// Target already has a manager from previous run
		TargetHasDirectManager = 4,		// Target has another direct manager that will be set in this run
	};
}

/**
 * Asset Registry module interface
 */
class COREUOBJECT_API IAssetRegistryInterface
{
public:
	/**
	 * Tries to gets a pointer to the active AssetRegistryInterface implementation. 
	 */
	static IAssetRegistryInterface* GetPtr();

	UE_DEPRECATED(4.26, "Use GetDependencies that takes a UE::AssetRegistry::EDependencyCategory instead")
	void GetDependencies(FName InPackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType)
	{
		GetDependenciesDeprecated(InPackageName, OutDependencies, InDependencyType);
	}

	/**
	 * Lookup dependencies for the given package name and fill OutDependencies with direct dependencies
	 */
	virtual void GetDependencies(FName InPackageName, TArray<FName>& OutDependencies, UE::AssetRegistry::EDependencyCategory Category = UE::AssetRegistry::EDependencyCategory::Package, const UE::AssetRegistry::FDependencyQuery& Flags = UE::AssetRegistry::FDependencyQuery()) = 0;

protected:
	/* This function is a workaround for platforms that don't support disable of deprecation warnings on override functions*/
	virtual void GetDependenciesDeprecated(FName InPackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType) = 0;

	static IAssetRegistryInterface* Default;
	friend class UAssetRegistryImpl;
};
