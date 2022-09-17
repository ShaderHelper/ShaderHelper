// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/ObjectMacros.h"

struct FAssetData;

/**
 * A struct to serve as a filter for Asset Registry queries.
 * Each component element is processed as an 'OR' operation while all the components are processed together as an 'AND' operation.
 */
struct FARFilter
{
	/** The filter component for package names */
	TArray<FName> PackageNames;

	/** The filter component for package paths */
	TArray<FName> PackagePaths;

	/** The filter component containing specific object paths */
	TArray<FName> ObjectPaths;

	/** The filter component for class names. Instances of the specified classes, but not subclasses (by default), will be included. Derived classes will be included only if bRecursiveClasses is true. */
	TArray<FName> ClassNames;

	/** The filter component for properties marked with the AssetRegistrySearchable flag */
	TMultiMap<FName, TOptional<FString>> TagsAndValues;

	/** Only if bRecursiveClasses is true, the results will exclude classes (and subclasses) in this list */
	TSet<FName> RecursiveClassesExclusionSet;

	/** If true, PackagePath components will be recursive */
	bool bRecursivePaths = false;

	/** If true, subclasses of ClassNames will also be included and RecursiveClassesExclusionSet will be excluded. */
	bool bRecursiveClasses = false;

	/** If true, only on-disk assets will be returned. Be warned that this is rarely what you want and should only be used for performance reasons */
	bool bIncludeOnlyOnDiskAssets = false;

	/** The exclusive filter component for package flags. Only assets without any of the specified flags will be returned. */
	uint32 WithoutPackageFlags = 0;

	/** The inclusive filter component for package flags. Only assets with all of the specified flags will be returned. */
	uint32 WithPackageFlags = 0;

	/** Appends the other filter to this one */
	void Append(const FARFilter& Other)
	{
		PackageNames.Append(Other.PackageNames);
		PackagePaths.Append(Other.PackagePaths);
		ObjectPaths.Append(Other.ObjectPaths);
		ClassNames.Append(Other.ClassNames);

		for (auto TagIt = Other.TagsAndValues.CreateConstIterator(); TagIt; ++TagIt)
		{
			TagsAndValues.Add(TagIt.Key(), TagIt.Value());
		}

		RecursiveClassesExclusionSet.Append(Other.RecursiveClassesExclusionSet);

		bRecursivePaths |= Other.bRecursivePaths;
		bRecursiveClasses |= Other.bRecursiveClasses;
		bIncludeOnlyOnDiskAssets |= Other.bIncludeOnlyOnDiskAssets;
		WithoutPackageFlags |= Other.WithoutPackageFlags;
		WithPackageFlags |= Other.WithPackageFlags;
	}

	/** Returns true if this filter has no entries */
	bool IsEmpty() const
	{
		return PackageNames.Num() + PackagePaths.Num() + ObjectPaths.Num() + ClassNames.Num() + TagsAndValues.Num() + WithoutPackageFlags + WithPackageFlags == 0;
	}

	/** Returns true if this filter is recursive */
	bool IsRecursive() const
	{
		return bRecursivePaths || bRecursiveClasses;
	}

	/** Clears this filter of all entries */
	void Clear()
	{
		PackageNames.Empty();
		PackagePaths.Empty();
		ObjectPaths.Empty();
		ClassNames.Empty();
		TagsAndValues.Empty();
		RecursiveClassesExclusionSet.Empty();

		bRecursivePaths = false;
		bRecursiveClasses = false;
		bIncludeOnlyOnDiskAssets = false;
		WithoutPackageFlags = 0;
		WithPackageFlags = 0;

		ensure(IsEmpty());
	}
};
/**
 * A struct to serve as a filter for Asset Registry queries.
 * Each component element is processed as an 'OR' operation while all the components are processed together as an 'AND' operation.
 * This is a version of FARFilter optimized for querying, and can be generated from an FARFilter by calling IAssetRegistry::CompileFilter to resolve any recursion.
 */
struct FARCompiledFilter
{
	/** The filter component for package names */
	TSet<FName> PackageNames;

	/** The filter component for package paths */
	TSet<FName> PackagePaths;

	/** The filter component containing specific object paths */
	TSet<FName> ObjectPaths;

	/** The filter component for class names. Instances of the specified classes, but not subclasses (by default), will be included. Derived classes will be included only if bRecursiveClasses is true. */
	TSet<FName> ClassNames;

	/** The filter component for properties marked with the AssetRegistrySearchable flag */
	TMultiMap<FName, TOptional<FString>> TagsAndValues;

	/** The exclusive filter component for package flags. Only assets without any of the specified flags will be returned. */
	uint32 WithoutPackageFlags = 0;

	/** The inclusive filter component for package flags. Only assets with all of the specified flags will be returned. */
	uint32 WithPackageFlags = 0;

	/** If true, only on-disk assets will be returned. Be warned that this is rarely what you want and should only be used for performance reasons */
	bool bIncludeOnlyOnDiskAssets = false;

	/** Returns true if this filter has no entries */
	bool IsEmpty() const
	{
		return PackageNames.Num() + PackagePaths.Num() + ObjectPaths.Num() + ClassNames.Num() + TagsAndValues.Num() == 0;
	}

	/** Clears this filter of all entries */
	void Clear()
	{
		PackageNames.Empty();
		PackagePaths.Empty();
		ObjectPaths.Empty();
		ClassNames.Empty();
		TagsAndValues.Empty();

		bIncludeOnlyOnDiskAssets = false;
		WithoutPackageFlags = 0;
		WithPackageFlags = 0;

		ensure(IsEmpty());
	}
};
