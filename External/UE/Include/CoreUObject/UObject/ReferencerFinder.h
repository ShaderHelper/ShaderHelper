// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ArchiveUObject.h"

enum class EReferencerFinderFlags : uint8
{
	None = 0,
	SkipInnerReferences = 1, // Do not add inner objects to the referencers of an outer.
};

ENUM_CLASS_FLAGS(EReferencerFinderFlags);

/**
 * Helper class for finding all objects referencing any of the objects in Referencees list
 */
class COREUOBJECT_API FReferencerFinder
{
public:
	static TArray<UObject*> GetAllReferencers(const TArray<UObject*>& Referencees, const TSet<UObject*>* ObjectsToIgnore, EReferencerFinderFlags Flags = EReferencerFinderFlags::None);
	static TArray<UObject*> GetAllReferencers(const TSet<UObject*>& Referencees, const TSet<UObject*>* ObjectsToIgnore, EReferencerFinderFlags Flags = EReferencerFinderFlags::None);

	/** Called when the initial load phase is complete and we're done registering native UObject classes */
	static void NotifyRegistrationComplete();
};