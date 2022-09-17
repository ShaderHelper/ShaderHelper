// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AssetRegistry/AssetBundleData.h"
#include "AssetRegistry/AssetDataTagMap.h"
#include "Containers/ArrayView.h"
#include "Containers/StringView.h"
#include "Misc/PackageName.h"
#include "Misc/StringBuilder.h"
#include "Templates/UniquePtr.h"
#include "UObject/Class.h"
#include "UObject/LinkerLoad.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/PrimaryAssetId.h"

struct FCustomVersion;

COREUOBJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogAssetData, Log, All);

/** Version used for serializing asset registry caches, both runtime and editor */
struct COREUOBJECT_API FAssetRegistryVersion
{
	enum Type
	{
		PreVersioning = 0,					// From before file versioning was implemented
		HardSoftDependencies,				// The first version of the runtime asset registry to include file versioning.
		AddAssetRegistryState,				// Added FAssetRegistryState and support for piecemeal serialization
		ChangedAssetData,					// AssetData serialization format changed, versions before this are not readable
		RemovedMD5Hash,						// Removed MD5 hash from package data
		AddedHardManage,					// Added hard/soft manage references
		AddedCookedMD5Hash,					// Added MD5 hash of cooked package to package data
		AddedDependencyFlags,				// Added UE::AssetRegistry::EDependencyProperty to each dependency
		FixedTags,							// Major tag format change that replaces USE_COMPACT_ASSET_REGISTRY:
											// * Target tag INI settings cooked into tag data
											// * Instead of FString values are stored directly as one of:
											//		- Narrow / wide string
											//		- [Numberless] FName
											//		- [Numberless] export path
											//		- Localized string
											// * All value types are deduplicated
											// * All key-value maps are cooked into a single contiguous range 
											// * Switched from FName table to seek-free and more optimized FName batch loading
											// * Removed global tag storage, a tag map reference-counts one store per asset registry
											// * All configs can mix fixed and loose tag maps 
		WorkspaceDomain,					// Added Version information to AssetPackageData
		PackageImportedClasses,				// Added ImportedClasses to AssetPackageData
		PackageFileSummaryVersionChange,	// A new version number of UE5 was added to FPackageFileSummary
		ObjectResourceOptionalVersionChange,// Change to linker export/import resource serializationn

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	/** The GUID for this custom version number */
	const static FGuid GUID;

	/** Read/write the custom version to the archive, should call at the very beginning */
	static bool SerializeVersion(FArchive& Ar, FAssetRegistryVersion::Type& Version);

private:
	FAssetRegistryVersion() {}
};

/** 
 * A struct to hold important information about an assets found by the Asset Registry
 * This struct is transient and should never be serialized
 */
struct FAssetData
{
public:
	/** The prefix used for collection entries inside TagsAndValues */
	static const TCHAR* GetCollectionTagPrefix()
	{
		return TEXT("CL_");
	}

	enum class ECreationFlags
	{
		None = 0,
		SkipAssetRegistryTagsGathering = 1 << 0, // Do not perform expensive step of gathering asset registry tags at construction.
		AllowBlueprintClass            = 1 << 1, // Unless specified, the default when trying to create one for a blueprint class 
												 // will create one for the UBlueprint instead, but this can be overridden
	};

public:
	/** The object path for the asset in the form PackageName.ObjectName, or PackageName.ObjectName:SubObjectName. */
	FName ObjectPath;
	/** The name of the package in which the asset is found, this is the full long package name such as /Game/Path/Package */
	FName PackageName;
	/** The path to the package in which the asset is found, this is /Game/Path with the Package stripped off */
	FName PackagePath;
	/** The name of the asset without the package */
	FName AssetName;
	/** The name of the asset's class */
	FName AssetClass;
	/** The map of values for properties that were marked AssetRegistrySearchable or added by GetAssetRegistryTags */
	FAssetDataTagMapSharedView TagsAndValues;
	/**
	 * The 'AssetBundles' tag key is separated from TagsAndValues and typed for performance reasons.
	 * This is likely a temporary solution that will be generalized in some other fashion. 	
	 */
	TSharedPtr<FAssetBundleData, ESPMode::ThreadSafe> TaggedAssetBundles;

	COREUOBJECT_API void SetTagsAndAssetBundles(FAssetDataTagMap&& Tags);

	/** The IDs of the pakchunks this asset is located in for streaming install.  Empty if not assigned to a chunk */
	TArray<int32, TInlineAllocator<2>> ChunkIDs;
	/** Asset package flags */
	uint32 PackageFlags = 0;

public:
	/** Default constructor */
	FAssetData() {}

	/** Constructor building the ObjectPath in the form of InPackageName.InAssetName. does not work for object outer-ed to a different package. */
	COREUOBJECT_API FAssetData(FName InPackageName, FName InPackagePath, FName InAssetName, FName InAssetClass, FAssetDataTagMap InTags = FAssetDataTagMap(), TArrayView<const int32> InChunkIDs = TArrayView<const int32>(), uint32 InPackageFlags = 0);
	/** Constructor with a long package name and a full object path which might not be part of the package this asset is in. */
	COREUOBJECT_API FAssetData(const FString& InLongPackageName, const FString& InObjectPath, FName InAssetClass, FAssetDataTagMap InTags = FAssetDataTagMap(), TArrayView<const int32> InChunkIDs = TArrayView<const int32>(), uint32 InPackageFlags = 0);

	/** Constructor taking a UObject. By default trying to create one for a blueprint class will create one for the UBlueprint instead, but this can be overridden */
	COREUOBJECT_API FAssetData(const UObject* InAsset, FAssetData::ECreationFlags InCreationFlags = ECreationFlags::None);

	/** Constructor taking a UObject. By default trying to create one for a blueprint class will create one for the UBlueprint instead, but this can be overridden */
	inline FAssetData(const UObject* InAsset, bool bAllowBlueprintClass)
		: FAssetData(InAsset, bAllowBlueprintClass ? ECreationFlags::AllowBlueprintClass : ECreationFlags::None)
	{
	}

	/** FAssetDatas are equal if their object paths match */
	bool operator==(const FAssetData& Other) const
	{
		return ObjectPath == Other.ObjectPath;
	}

	bool operator!=(const FAssetData& Other) const
	{
		return ObjectPath != Other.ObjectPath;
	}

	bool operator>(const FAssetData& Other) const
	{
		return  Other.ObjectPath.LexicalLess(ObjectPath);
	}

	bool operator<(const FAssetData& Other) const
	{
		return ObjectPath.LexicalLess(Other.ObjectPath);
	}

	/** Checks to see if this AssetData refers to an asset or is NULL */
	bool IsValid() const
	{
		return !ObjectPath.IsNone();
	}

	/**
	 * Returns true if this is the main asset in a package, true for maps and assets but false for secondary objects like class redirectors
	 * Every UAsset is also a TopLevelAsset.
	 */
	bool IsUAsset() const
	{
		if (!IsValid())
		{
			return false;
		}

		TStringBuilder<FName::StringBufferSize> AssetNameStrBuilder;
		AssetName.ToString(AssetNameStrBuilder);

		TStringBuilder<FName::StringBufferSize> PackageNameStrBuilder;
		PackageName.ToString(PackageNameStrBuilder);
		return DetectIsUAssetByNames(PackageNameStrBuilder, AssetNameStrBuilder);
	}

	/**
	 * Returns true if the given UObject is the main asset in a package, true for maps and assets but false for secondary objects like class redirectors
	 * Every UAsset is also a TopLevelAsset.
	 */
	COREUOBJECT_API static bool IsUAsset(UObject* Object);

	/**
	 * Returns true iff the Asset is a TopLevelAsset (not a subobject, its outer is a UPackage).
	 * Only TopLevelAssets can be PrimaryAssets in the AssetManager.
	 * A TopLevelAsset is not necessarily the main asset in a package; see IsUAsset.
	 */
	COREUOBJECT_API bool IsTopLevelAsset() const;
	
	/**
	 * Returns true iff the given Object, assumed to be an Asset, is a TopLevelAsset (not a subobject, its outer is a UPackage).
	 * Only TopLevelAssets can be PrimaryAssets in the AssetManager.
	 * A TopLevelAsset is not necessarily the main asset in a package; see IsUAsset.
	 */
	COREUOBJECT_API static bool IsTopLevelAsset(UObject* Object);

	void Shrink()
	{
		ChunkIDs.Shrink();
		TagsAndValues.Shrink();
	}

	/** Returns the full name for the asset in the form: Class ObjectPath */
	FString GetFullName() const
	{
		FString FullName;
		GetFullName(FullName);
		return FullName;
	}

	/** Populates OutFullName with the full name for the asset in the form: Class ObjectPath */
	void GetFullName(FString& OutFullName) const
	{
		OutFullName.Reset();
		AssetClass.AppendString(OutFullName);
		OutFullName.AppendChar(' ');
		ObjectPath.AppendString(OutFullName);
	}

	/** Populates OutFullNameBuilder with the full name for the asset in the form: Class ObjectPath */
	void GetFullName(FStringBuilderBase& OutFullNameBuilder) const
	{
		OutFullNameBuilder.Reset();
		AssetClass.AppendString(OutFullNameBuilder);
		OutFullNameBuilder.AppendChar(' ');
		ObjectPath.AppendString(OutFullNameBuilder);
	}

	/** Returns the name for the asset in the form: Class'ObjectPath' */
	FString GetExportTextName() const
	{
		FString ExportTextName;
		GetExportTextName(ExportTextName);
		return ExportTextName;
	}

	/** Populates OutExportTextName with the name for the asset in the form: Class'ObjectPath' */
	void GetExportTextName(FString& OutExportTextName) const
	{
		OutExportTextName.Reset();
		AssetClass.AppendString(OutExportTextName);
		OutExportTextName.AppendChar('\'');
		ObjectPath.AppendString(OutExportTextName);
		OutExportTextName.AppendChar('\'');
	}

	/** Populates OutExportTextNameBuilder with the name for the asset in the form: Class'ObjectPath' */
	void GetExportTextName(FStringBuilderBase& OutExportTextNameBuilder) const
	{
		OutExportTextNameBuilder.Reset();
		AssetClass.AppendString(OutExportTextNameBuilder);
		OutExportTextNameBuilder.AppendChar('\'');
		ObjectPath.AppendString(OutExportTextNameBuilder);
		OutExportTextNameBuilder.AppendChar('\'');
	}

	/** Returns true if the this asset is a redirector. */
	bool IsRedirector() const
	{
		return IsRedirectorClassName(AssetClass);
	}

	static bool IsRedirector(UObject* Object)
	{
		return Object && IsRedirectorClassName(Object->GetClass()->GetFName());
	}

	/** Returns the class UClass if it is loaded. It is not possible to load the class if it is unloaded since we only have the short name. */
	UClass* GetClass() const
	{
		if ( !IsValid() )
		{
			// Dont even try to find the class if the objectpath isn't set
			return NULL;
		}

		UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *AssetClass.ToString());

		if (!FoundClass)
		{
			// Look for class redirectors
			FName NewPath = FLinkerLoad::FindNewNameForClass(AssetClass, false);

			if (NewPath != NAME_None)
			{
				FoundClass = FindObject<UClass>(ANY_PACKAGE, *NewPath.ToString());
			}
		}
		return FoundClass;
	}
	
	/** Returns whether the Asset's class is equal to or a child class of the given class. Returns false if the Asset's class can not be loaded. */
	bool IsInstanceOf(const UClass* BaseClass) const
	{
		UClass* ClassPointer = GetClass();
		return ClassPointer && ClassPointer->IsChildOf(BaseClass);
	}

	/** Convert to a SoftObjectPath for loading */
	FSoftObjectPath ToSoftObjectPath() const
	{
		return FSoftObjectPath(ObjectPath);
	}

	UE_DEPRECATED(4.18, "ToStringReference was renamed to ToSoftObjectPath")
	FSoftObjectPath ToStringReference() const
	{
		return ToSoftObjectPath();
	}
	
	/** Gets primary asset id of this data */
	COREUOBJECT_API FPrimaryAssetId GetPrimaryAssetId() const;

	/** Returns the asset UObject if it is loaded or loads the asset if it is unloaded then returns the result */
	UObject* FastGetAsset(bool bLoad=false) const
	{
		if ( !IsValid())
		{
			// Do not try to find the object if the objectpath is not set
			return nullptr;
		}

		UPackage* FoundPackage = FindObjectFast<UPackage>(nullptr, PackageName);
		if (FoundPackage == nullptr)
		{
			if (bLoad)
			{
				return LoadObject<UObject>(nullptr, *ObjectPath.ToString());
			}
			else
			{
				return nullptr;
			}
		}

		UObject* Asset = FindObjectFast<UObject>(FoundPackage, AssetName);
		if (Asset == nullptr && bLoad)
		{
			return LoadObject<UObject>(nullptr, *ObjectPath.ToString());
		}

		return Asset;
	}

	/** Returns the asset UObject if it is loaded or loads the asset if it is unloaded then returns the result */
	UObject* GetAsset() const
	{
		if ( !IsValid())
		{
			// Dont even try to find the object if the objectpath isn't set
			return nullptr;
		}

		UObject* Asset = FindObject<UObject>(nullptr, *ObjectPath.ToString());
		if ( Asset == nullptr)
		{
			Asset = LoadObject<UObject>(nullptr, *ObjectPath.ToString());
		}

		return Asset;
	}

	/**
	 * Used to check whether the any of the passed flags are set in the cached asset package flags.
	 * @param	FlagsToCheck  Package flags to check for
	 * @return	true if any of the passed in flag are set, false otherwise
	 * @see UPackage::HasAnyPackageFlags
	 */
	bool HasAnyPackageFlags(uint32 FlagsToCheck) const
	{
		return (PackageFlags & FlagsToCheck) != 0;
	}

	/**
	 * Used to check whether all of the passed flags are set in the cached asset package flags.
	 * @param FlagsToCheck	Package flags to check for
	 * @return true if all of the passed in flags are set (including no flags passed in), false otherwise
	 * @see UPackage::HasAllPackagesFlags
	 */
	bool HasAllPackageFlags(uint32 FlagsToCheck) const
	{
		return ((PackageFlags & FlagsToCheck) == FlagsToCheck);
	}

	UPackage* GetPackage() const
	{
		if (PackageName == NAME_None)
		{
			return NULL;
		}

		UPackage* Package = FindPackage(NULL, *PackageName.ToString());
		if (Package)
		{
			Package->FullyLoad();
		}
		else
		{
			Package = LoadPackage(NULL, *PackageName.ToString(), LOAD_None);
		}

		return Package;
	}

	/** Try to find the given tag  */
	bool FindTag(const FName InTagName) const
	{
		return TagsAndValues.FindTag(InTagName).IsSet();
	}

	/** Try and get the value associated with the given tag as a type converted value */
	template <typename ValueType>
	bool GetTagValue(FName Tag, ValueType& OutValue) const;

	/** Try and get the value associated with the given tag as a type converted value, or an empty value if it doesn't exist */
	template <typename ValueType>
	ValueType GetTagValueRef(const FName Tag) const;

	/** Returns true if the asset is loaded */
	bool IsAssetLoaded() const
	{
		return IsValid() && FindObjectSafe<UObject>(NULL, *ObjectPath.ToString()) != NULL;
	}

	/** Prints the details of the asset to the log */
	void PrintAssetData() const
	{
		UE_LOG(LogAssetData, Log, TEXT("    FAssetData for %s"), *ObjectPath.ToString());
		UE_LOG(LogAssetData, Log, TEXT("    ============================="));
		UE_LOG(LogAssetData, Log, TEXT("        PackageName: %s"), *PackageName.ToString());
		UE_LOG(LogAssetData, Log, TEXT("        PackagePath: %s"), *PackagePath.ToString());
		UE_LOG(LogAssetData, Log, TEXT("        AssetName: %s"), *AssetName.ToString());
		UE_LOG(LogAssetData, Log, TEXT("        AssetClass: %s"), *AssetClass.ToString());
		UE_LOG(LogAssetData, Log, TEXT("        TagsAndValues: %d"), TagsAndValues.Num());

		for (const auto TagValue: TagsAndValues)
		{
			UE_LOG(LogAssetData, Log, TEXT("            %s : %s"), *TagValue.Key.ToString(), *TagValue.Value.AsString());
		}

		UE_LOG(LogAssetData, Log, TEXT("        ChunkIDs: %d"), ChunkIDs.Num());

		for (int32 Chunk: ChunkIDs)
		{
			UE_LOG(LogAssetData, Log, TEXT("                 %d"), Chunk);
		}

		UE_LOG(LogAssetData, Log, TEXT("        PackageFlags: %d"), PackageFlags);
	}

	/** Get the first FAssetData of a particular class from an Array of FAssetData */
	static FAssetData GetFirstAssetDataOfClass(const TArray<FAssetData>& Assets, const UClass* DesiredClass)
	{
		for(int32 AssetIdx=0; AssetIdx<Assets.Num(); AssetIdx++)
		{
			const FAssetData& Data = Assets[AssetIdx];
			if (Data.IsInstanceOf(DesiredClass))
			{
				return Data;
			}
		}
		return FAssetData();
	}

	/** Convenience template for finding first asset of a class */
	template <class T>
	static T* GetFirstAsset(const TArray<FAssetData>& Assets)
	{
		UClass* DesiredClass = T::StaticClass();
		UObject* Asset = FAssetData::GetFirstAssetDataOfClass(Assets, DesiredClass).GetAsset(); //-V758
		check(Asset == NULL || Asset->IsA(DesiredClass));
		return (T*)Asset;
	}

	/** 
	 * Serialize as part of the registry cache. This is not meant to be serialized as part of a package so  it does not handle versions normally
	 * To version this data change FAssetRegistryVersion
	 */
	template<class Archive>
	void SerializeForCache(Archive&& Ar)
	{
		// Serialize out the asset info
		Ar << ObjectPath;
		Ar << PackagePath;
		Ar << AssetClass;

		// These are derived from ObjectPath, we manually serialize them because they get pooled
		Ar << PackageName;
		Ar << AssetName;

		Ar.SerializeTagsAndBundles(*this);

		Ar << ChunkIDs;
		Ar << PackageFlags;
	}

private:
	static bool DetectIsUAssetByNames(FStringView PackageName, FStringView ObjectPathName)
	{
		FStringView PackageBaseName;
		{
			// Get everything after the last slash
			int32 IndexOfLastSlash = INDEX_NONE;
			PackageName.FindLastChar(TEXT('/'), IndexOfLastSlash);
			PackageBaseName = PackageName.Mid(IndexOfLastSlash + 1);
		}

		return PackageBaseName.Equals(ObjectPathName, ESearchCase::IgnoreCase);
	}

	static bool IsRedirectorClassName(FName ClassName)
	{
		static const FName ObjectRedirectorClassName = UObjectRedirector::StaticClass()->GetFName();
		return ClassName == ObjectRedirectorClassName;
	}
};

ENUM_CLASS_FLAGS(FAssetData::ECreationFlags);

FORCEINLINE uint32 GetTypeHash(const FAssetData& AssetData)
{
	return GetTypeHash(AssetData.ObjectPath);
}


template<>
struct TStructOpsTypeTraits<FAssetData> : public TStructOpsTypeTraitsBase2<FAssetData>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};

template <typename ValueType>
inline bool FAssetData::GetTagValue(FName Tag, ValueType& OutValue) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	if (FoundValue.IsSet())
	{
		FMemory::Memzero(&OutValue, sizeof(ValueType));
		LexFromString(OutValue, *FoundValue.GetValue());
		return true;
	}
	return false;
}

template <>
inline bool FAssetData::GetTagValue<FString>(FName Tag, FString& OutValue) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	if (FoundValue.IsSet())
	{
		OutValue = FoundValue.AsString();
		return true;
	}

	return false;
}

template <>
inline bool FAssetData::GetTagValue<FText>(FName Tag, FText& OutValue) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	if (FoundValue.IsSet())
	{
		OutValue = FoundValue.AsText();
		return true;
	}

	return false;
}

template <>
inline bool FAssetData::GetTagValue<FName>(FName Tag, FName& OutValue) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	if (FoundValue.IsSet())
	{
		OutValue = FoundValue.AsName();
		return true;
	}
	return false;
}

template <typename ValueType>
inline ValueType FAssetData::GetTagValueRef(FName Tag) const
{
	ValueType TmpValue;
	FMemory::Memzero(&TmpValue, sizeof(ValueType));
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	if (FoundValue.IsSet())
	{
		LexFromString(TmpValue, *FoundValue.GetValue());
	}
	return TmpValue;
}

template <>
inline FString FAssetData::GetTagValueRef<FString>(FName Tag) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	return FoundValue.IsSet() ? FoundValue.AsString() : FString();
}

template <>
inline FText FAssetData::GetTagValueRef<FText>(FName Tag) const
{
	FText TmpValue;
	GetTagValue(Tag, TmpValue);
	return TmpValue;
}

template <>
inline FName FAssetData::GetTagValueRef<FName>(FName Tag) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	return FoundValue.IsSet() ? FoundValue.AsName() : FName();
}

template <>
inline FAssetRegistryExportPath FAssetData::GetTagValueRef<FAssetRegistryExportPath>(FName Tag) const
{
	const FAssetDataTagMapSharedView::FFindTagResult FoundValue = TagsAndValues.FindTag(Tag);
	return FoundValue.IsSet() ? FoundValue.AsExportPath() : FAssetRegistryExportPath();
}

namespace UE
{
namespace AssetRegistry
{

/** Low-memory version of FCustomVersion; holds only Guid and integer version. */
struct FPackageCustomVersion
{
	FGuid Key;
	int32 Version = 0;

	FPackageCustomVersion() = default;
	FPackageCustomVersion(const FGuid& InKey, const int32 InVersion)
		: Key(InKey)
		, Version(InVersion)
	{
	}
	bool operator<(const FPackageCustomVersion& RHS) const
	{
		if (Key < RHS.Key) return true;
		if (RHS.Key < Key) return false;
		return Version < RHS.Version;
	}
	bool operator==(const FPackageCustomVersion& RHS) const
	{
		return Key == RHS.Key && Version == RHS.Version;
	}
	friend FArchive& operator<<(FArchive& Ar, FPackageCustomVersion& CustomVersion)
	{
		return Ar << CustomVersion.Key << CustomVersion.Version;
	}
};

/** A handle to a deduplicated, sorted array of FPackageCustomVersion. */
class FPackageCustomVersionsHandle
{
public:
	TConstArrayView<FPackageCustomVersion> Get() const { return Ptr; }

	COREUOBJECT_API static FPackageCustomVersionsHandle FindOrAdd(TConstArrayView<FCustomVersion> InCustomVersions);
	COREUOBJECT_API static FPackageCustomVersionsHandle FindOrAdd(TConstArrayView<FPackageCustomVersion> InCustomVersions);
	COREUOBJECT_API static FPackageCustomVersionsHandle FindOrAdd(TArray<FPackageCustomVersion>&& InCustomVersions);
	COREUOBJECT_API friend FArchive& operator<<(FArchive& Ar, FPackageCustomVersionsHandle& Handle);

private:
	TConstArrayView<FPackageCustomVersion> Ptr;
	friend class FPackageCustomVersionRegistry;
};

}
}

/** A class to hold data about a package on disk, this data is updated on save/load and is not updated when an asset changes in memory */
PRAGMA_DISABLE_DEPRECATION_WARNINGS // Silence deprecation warnings for deprecated PackageGuid member in implicit constructors
class FAssetPackageData
{
public:
	/** Guid of the source package, uniquely identifies an asset package */
	UE_DEPRECATED(4.27, "UPackage::Guid has not been used by the engine for a long time and FAssetPackageData::PackageGuid will be removed.")
	FGuid PackageGuid;

	/** MD5 of the cooked package on disk, for tracking nondeterministic changes */
	FMD5Hash CookedHash;

	/** List of classes used by exports in the package. Does not include classes in the same package. */
	TArray<FName> ImportedClasses;

	/** Total size of this asset on disk */
	int64 DiskSize;

	/** UE file version that the package was saved with */
	FPackageFileVersion FileVersionUE;

	/** Licensee file version that the package was saved with */
	int32 FileVersionLicenseeUE;

private:
	UE::AssetRegistry::FPackageCustomVersionsHandle CustomVersions;
	/** Bit storage for flags */
	uint32 Flags;

public:

	FAssetPackageData()
		: DiskSize(0)
		, FileVersionLicenseeUE(-1)
		, Flags(0)
	{
	}

	/**
	 * Custom versions used by the package, used to check whether we need to update the package for the current binary.
	 * The array is sorted by FPackageCustomVersion::operator<.
	 */
	TConstArrayView<UE::AssetRegistry::FPackageCustomVersion> GetCustomVersions() const
	{
		return CustomVersions.Get();
	}
	void SetCustomVersions(TConstArrayView<FCustomVersion> InCustomVersions)
	{
		CustomVersions = UE::AssetRegistry::FPackageCustomVersionsHandle::FindOrAdd(InCustomVersions);
	}
	void SetCustomVersions(TConstArrayView<UE::AssetRegistry::FPackageCustomVersion> InCustomVersions)
	{
		CustomVersions = UE::AssetRegistry::FPackageCustomVersionsHandle::FindOrAdd(InCustomVersions);
	}
	void SetCustomVersions(TArray<UE::AssetRegistry::FPackageCustomVersion>&& InCustomVersions)
	{
		CustomVersions = UE::AssetRegistry::FPackageCustomVersionsHandle::FindOrAdd(MoveTemp(InCustomVersions));
	}

	/** Whether the package was saved from a licensee executable, used to tell whether non-matching FileVersionLicenseeUE requires a resave */
	bool IsLicenseeVersion() const { return (Flags & FLAG_LICENSEE_VERSION) != 0; }
	void SetIsLicenseeVersion(bool bValue) { Flags = (Flags & ~FLAG_LICENSEE_VERSION) | (bValue ? FLAG_LICENSEE_VERSION : 0); }

	/**
	 * Serialize as part of the registry cache. This is not meant to be serialized as part of a package so  it does not handle versions normally
	 * To version this data change FAssetRegistryVersion
	 */
	void SerializeForCache(FArchive& Ar)
	{
		// Calling with hard-coded version and using force-inline on SerializeForCacheInternal eliminates the cost of its if-statements
		SerializeForCacheInternal(Ar, FAssetRegistryVersion::LatestVersion);
	}
	void SerializeForCacheOldVersion(FArchive& Ar, FAssetRegistryVersion::Type Version)
	{
		SerializeForCacheInternal(Ar, Version);
	}

private:
	enum
	{
		FLAG_LICENSEE_VERSION = 0x1,
	};

	void SerializeForCacheInternal(FArchive& Ar, FAssetRegistryVersion::Type Version);
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS
FORCEINLINE void FAssetPackageData::SerializeForCacheInternal(FArchive& Ar, FAssetRegistryVersion::Type Version)
{
	Ar << DiskSize;
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	Ar << PackageGuid;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	if (Version >= FAssetRegistryVersion::AddedCookedMD5Hash)
	{
		Ar << CookedHash;
	}
	if (Version >= FAssetRegistryVersion::WorkspaceDomain)
	{
		if (Version >= FAssetRegistryVersion::PackageFileSummaryVersionChange)
		{
			Ar << FileVersionUE;
		}
		else
		{
			int32 UE4Version;
			Ar << UE4Version;

			FileVersionUE = FPackageFileVersion::CreateUE4Version(UE4Version);
		}

		Ar << FileVersionLicenseeUE;
		Ar << Flags;
		Ar << CustomVersions;
	}
	if (Version >= FAssetRegistryVersion::PackageImportedClasses)
	{
		Ar << ImportedClasses;
	}
}


/**
 * Helper struct for FAssetIdentifier (e.g., for the FOnViewAssetIdentifiersInReferenceViewer delegate and Reference Viewer functions).
 */
#if WITH_EDITORONLY_DATA
struct FReferenceViewerParams
{
	FReferenceViewerParams()
		// Displayed-on-graph options
		: bShowReferencers(true)
		, bShowDependencies(true)
		// Slider-based options
		, FixAndHideSearchDepthLimit(0)
		, FixAndHideSearchBreadthLimit(0)
		, bShowCollectionFilter(true)
		// Checkbox options
		, bShowShowReferencesOptions(true)
		, bShowShowSearchableNames(true)
		, bShowShowNativePackages(true)
		, bShowShowFilteredPackagesOnly(true)
		, bShowCompactMode(true)
	{}

	/* Whether to display the Referencers */
	bool bShowReferencers;
	/* Whether to display the Dependencies */
	bool bShowDependencies;
	/* Whether to only display the References/Dependencies which match the text filter, if any. 
	   If the optional is not set, don't change the current reference viewer's value. */
	TOptional<bool> bShowFilteredPackagesOnly;
	/* Compact mode allows to hide the thumbnail and minimize the space taken by the nodes. Useful when there are many dependencies to inspect, to keep the UI responsive. 
	   If the optional is not set, don't change the current reference viewer's value. */
	TOptional<bool> bCompactMode;
	/**
	 * Whether to visually show to the user the option of "Search Depth Limit" or hide it and fix it to a default value:
	 * - If 0 or negative, it will show to the user the option of "Search Depth Limit".
	 * - If >0, it will hide that option and fix the Depth value to this value.
	 */
	int32 FixAndHideSearchDepthLimit;
	/**
	 * Whether to visually show to the user the option of "Search Breadth Limit" or hide it and fix it to a default value:
	 * - If 0 or negative, it will show to the user the option of "Search Breadth Limit".
	 * - If >0, it will hide that option and fix the Breadth value to this value.
	 */
	int32 FixAndHideSearchBreadthLimit;
	/** Whether to visually show to the user the option of "Collection Filter" */
	bool bShowCollectionFilter;
	/** Whether to visually show to the user the options of "Show Soft/Hard/Management References" */
	bool bShowShowReferencesOptions;
	/** Whether to visually show to the user the option of "Show Searchable Names" */
	bool bShowShowSearchableNames;
	/** Whether to visually show to the user the option of "Show Native Packages" */
	bool bShowShowNativePackages;
	/** Whether to visually show to the user the option of "Show Filtered Packages Only" */
	bool bShowShowFilteredPackagesOnly;
	/** Whether to visually show to the user the option of "Compact Mode" */
	bool bShowCompactMode;
};
#endif // WITH_EDITORONLY_DATA

/** A structure defining a thing that can be reference by something else in the asset registry. Represents either a package of a primary asset id */
struct FAssetIdentifier
{
	/** The name of the package that is depended on, this is always set unless PrimaryAssetType is */
	FName PackageName;
	/** The primary asset type, if valid the ObjectName is the PrimaryAssetName */
	FPrimaryAssetType PrimaryAssetType;
	/** Specific object within a package. If empty, assumed to be the default asset */
	FName ObjectName;
	/** Name of specific value being referenced, if ObjectName specifies a type such as a UStruct */
	FName ValueName;

	/** Can be implicitly constructed from just the package name */
	FAssetIdentifier(FName InPackageName, FName InObjectName = FName(), FName InValueName = FName())
		: PackageName(InPackageName), PrimaryAssetType(), ObjectName(InObjectName), ValueName(InValueName)
	{}

	/** Construct from a primary asset id */
	FAssetIdentifier(const FPrimaryAssetId& PrimaryAssetId, FName InValueName = FName())
		: PackageName(), PrimaryAssetType(PrimaryAssetId.PrimaryAssetType), ObjectName(PrimaryAssetId.PrimaryAssetName), ValueName(InValueName)
	{}

	FAssetIdentifier(UObject* SourceObject, FName InValueName)
	{
		if (SourceObject)
		{
			UPackage* Package = SourceObject->GetOutermost();
			PackageName = Package->GetFName();
			ObjectName = SourceObject->GetFName();
			ValueName = InValueName;
		}
	}

	FAssetIdentifier() {}

	/** Returns primary asset id for this identifier, if valid */
	FPrimaryAssetId GetPrimaryAssetId() const
	{
		if (PrimaryAssetType != NAME_None)
		{
			return FPrimaryAssetId(PrimaryAssetType, ObjectName);
		}
		return FPrimaryAssetId();
	}

	/** Returns true if this represents a package */
	bool IsPackage() const
	{
		return PackageName != NAME_None && !IsObject() && !IsValue();
	}

	/** Returns true if this represents an object, true for both package objects and PrimaryAssetId objects */
	bool IsObject() const
	{
		return ObjectName != NAME_None && !IsValue();
	}

	/** Returns true if this represents a specific value */
	bool IsValue() const
	{
		return ValueName != NAME_None;
	}

	/** Returns true if this is a valid non-null identifier */
	bool IsValid() const
	{
		return PackageName != NAME_None || GetPrimaryAssetId().IsValid();
	}

	/** Returns string version of this identifier in Package.Object::Name format */
	FString ToString() const
	{
		TStringBuilder<256> Builder;
		AppendString(Builder);
		return FString(Builder.Len(), Builder.GetData());
	}

	/** Appends to the given builder the string version of this identifier in Package.Object::Name format */
	void AppendString(FStringBuilderBase& Builder) const
	{
		if (PrimaryAssetType != NAME_None)
		{
			GetPrimaryAssetId().AppendString(Builder);
		}
		else
		{
			PackageName.AppendString(Builder);
			if (ObjectName != NAME_None)
			{
				Builder.Append(TEXT("."));
				ObjectName.AppendString(Builder);
			}
		}
		if (ValueName != NAME_None)
		{
			Builder.Append(TEXT("::"));
			ValueName.AppendString(Builder);
		}
	}

	/** Converts from Package.Object::Name format */
	static FAssetIdentifier FromString(const FString& String)
	{
		// To right of :: is value
		FString PackageString;
		FString ObjectString;
		FString ValueString;

		// Try to split value out
		if (!String.Split(TEXT("::"), &PackageString, &ValueString))
		{
			PackageString = String;
		}

		// Check if it's a valid primary asset id
		FPrimaryAssetId PrimaryId = FPrimaryAssetId::FromString(PackageString);

		if (PrimaryId.IsValid())
		{
			return FAssetIdentifier(PrimaryId, *ValueString);
		}

		// Try to split on first . , if it fails PackageString will stay the same
		FString(PackageString).Split(TEXT("."), &PackageString, &ObjectString);

		return FAssetIdentifier(*PackageString, *ObjectString, *ValueString);
	}

	friend inline bool operator==(const FAssetIdentifier& A, const FAssetIdentifier& B)
	{
		return A.PackageName == B.PackageName && A.ObjectName == B.ObjectName && A.ValueName == B.ValueName;
	}

	friend inline uint32 GetTypeHash(const FAssetIdentifier& Key)
	{
		uint32 Hash = 0;

		// Most of the time only packagename is set
		if (Key.ObjectName.IsNone() && Key.ValueName.IsNone())
		{
			return GetTypeHash(Key.PackageName);
		}

		Hash = HashCombine(Hash, GetTypeHash(Key.PackageName));
		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetType));
		Hash = HashCombine(Hash, GetTypeHash(Key.ObjectName));
		Hash = HashCombine(Hash, GetTypeHash(Key.ValueName));
		return Hash;
	}

	/** Identifiers may be serialized as part of the registry cache, or in other contexts. If you make changes here you must also change FAssetRegistryVersion */
	friend FArchive& operator<<(FArchive& Ar, FAssetIdentifier& AssetIdentifier)
	{
		// Serialize bitfield of which elements to serialize, in general many are empty
		uint8 FieldBits = 0;

		if (Ar.IsSaving())
		{
			FieldBits |= (AssetIdentifier.PackageName != NAME_None) << 0;
			FieldBits |= (AssetIdentifier.PrimaryAssetType != NAME_None) << 1;
			FieldBits |= (AssetIdentifier.ObjectName != NAME_None) << 2;
			FieldBits |= (AssetIdentifier.ValueName != NAME_None) << 3;
		}

		Ar << FieldBits;

		if (FieldBits & (1 << 0))
		{
			Ar << AssetIdentifier.PackageName;
		}
		if (FieldBits & (1 << 1))
		{
			FName TypeName = AssetIdentifier.PrimaryAssetType.GetName();
			Ar << TypeName;

			if (Ar.IsLoading())
			{
				AssetIdentifier.PrimaryAssetType = TypeName;
			}
		}
		if (FieldBits & (1 << 2))
		{
			Ar << AssetIdentifier.ObjectName;
		}
		if (FieldBits & (1 << 3))
		{
			Ar << AssetIdentifier.ValueName;
		}
		
		return Ar;
	}
};

