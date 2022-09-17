// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "UObject/SoftObjectPtr.h"

/** A struct representing a single AssetBundle */
struct COREUOBJECT_API FAssetBundleEntry
{
	/** Declare constructors inline so this can be a header only class */
	FORCEINLINE FAssetBundleEntry() {}
	FORCEINLINE ~FAssetBundleEntry() {}


	/** Specific name of this bundle, should be unique for a given scope */
	FName BundleName;

	/** List of string assets contained in this bundle */
	TArray<FSoftObjectPath> BundleAssets;

	FAssetBundleEntry(const FAssetBundleEntry& OldEntry)
		: BundleName(OldEntry.BundleName), BundleAssets(OldEntry.BundleAssets)
	{

	}
	
	explicit FAssetBundleEntry(FName InBundleName)
		: BundleName(InBundleName)
	{

	}

	UE_DEPRECATED(4.27, "Bundles scopes are removed, please use FAssetBundleEntry(FName InBundleName) instead")
	FAssetBundleEntry(const FPrimaryAssetId& InBundleScope, FName InBundleName)
		: FAssetBundleEntry(InBundleName)
	{
		check(InBundleScope == FPrimaryAssetId());
	}

	/** Returns true if this represents a real entry */
	bool IsValid() const { return !BundleName.IsNone(); }


	/** Equality */
	bool operator==(const FAssetBundleEntry& Other) const
	{
		return BundleName == Other.BundleName && BundleAssets == Other.BundleAssets;
	}
	bool operator!=(const FAssetBundleEntry& Other) const
	{
		return !(*this == Other);
	}

	bool ExportTextItem(FString& ValueStr, const FAssetBundleEntry& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
};

/** A struct with a list of asset bundle entries. If one of these is inside a UObject it will get automatically exported as the asset registry tag AssetBundleData */
struct COREUOBJECT_API FAssetBundleData
{
	/** Declare constructors inline so this can be a header only class */
	FORCEINLINE FAssetBundleData() {}
	FORCEINLINE ~FAssetBundleData() {}
	FAssetBundleData(const FAssetBundleData&) = default;
	FAssetBundleData(FAssetBundleData&&) = default;
	FAssetBundleData& operator=(const FAssetBundleData&) = default;
	FAssetBundleData& operator=(FAssetBundleData&&) = default;

	/** List of bundles defined */
	TArray<FAssetBundleEntry> Bundles;

	/** Equality */
	bool operator==(const FAssetBundleData& Other) const
	{
		return Bundles == Other.Bundles;
	}
	bool operator!=(const FAssetBundleData& Other) const
	{
		return !(*this == Other);
	}

	/** Returns pointer to an entry with given Scope/Name */
	FAssetBundleEntry* FindEntry(FName SearchName);
	
	UE_DEPRECATED(4.27, "Bundles scopes are removed, please use FindEntry(FName) instead")
	FAssetBundleEntry* FindEntry(const FPrimaryAssetId& SearchScope, FName SearchName)
	{
		check(SearchScope == FPrimaryAssetId());
		return FindEntry(SearchName);
	}

	/** Adds or updates an entry with the given BundleName -> Path. Scope is empty and will be filled in later */
	void AddBundleAsset(FName BundleName, const FSoftObjectPath& AssetPath);

	template< typename T >
	void AddBundleAsset(FName BundleName, const TSoftObjectPtr<T>& SoftObjectPtr)
	{
		AddBundleAsset(BundleName, SoftObjectPtr.ToSoftObjectPath());
	}

	/** Adds multiple assets at once */
	void AddBundleAssets(FName BundleName, const TArray<FSoftObjectPath>& AssetPaths);

	/** A fast set of asset bundle assets, will destroy copied in path list */
	void SetBundleAssets(FName BundleName, TArray<FSoftObjectPath>&& AssetPaths);

	/** Resets the data to defaults */
	void Reset();

	/** Override Import/Export to not write out empty structs */
	bool ExportTextItem(FString& ValueStr, FAssetBundleData const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	FString ToDebugString() const;
};

template<>
struct TStructOpsTypeTraits<FAssetBundleData> : public TStructOpsTypeTraitsBase2<FAssetBundleData>
{
	enum
	{
		WithExportTextItem = true,
		WithImportTextItem = true,
	};
};
