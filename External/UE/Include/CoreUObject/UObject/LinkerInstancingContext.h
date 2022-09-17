// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Helper class to remap package imports during loading.
 * This is usually when objects in a package are outer-ed to object in another package or vice versa.
 * Instancing such a package without a instance remapping would resolve imports to the original package which is not desirable in an instancing context (i.e. loading a level instance)
 * This is because an instanced package has a different name than the package file name on disk, this class is used in the linker to remaps reference to the package name as stored in import tables on disk to the corresponding instanced package or packages we are loading.
 */
class FLinkerInstancingContext
{
public:
	FLinkerInstancingContext() = default;
	FLinkerInstancingContext(TMap<FName, FName> InInstanceMapping)
		: Mapping(MoveTemp(InInstanceMapping))
	{}

	bool IsInstanced() const
	{
		return Mapping.Num() > 0;
	}

	/** Remap the object name from the import table to its instanced counterpart, otherwise return the name unmodified. */
	FName Remap(const FName& ObjectName) const
	{
		if (const FName* RemappedName = Mapping.Find(ObjectName))
		{
			return *RemappedName;
		}
		return ObjectName;
	}

	void AddMapping(FName Original, FName Instanced)
	{
		Mapping.Add(Original, Instanced);
	}

	void AppendMapping(const TMap<FName, FName>& NewMapping)
	{
		Mapping.Append(NewMapping);
	}

private:
	friend class FLinkerLoad;

	/** Map of original object name to their instance counterpart. */
	TMap<FName, FName> Mapping;
};
