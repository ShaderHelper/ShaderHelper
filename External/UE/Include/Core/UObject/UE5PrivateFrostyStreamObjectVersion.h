// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in //UE5/Private-Frosty stream
struct CORE_API FUE5PrivateFrostyStreamObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		// Added HLODBatchingPolicy member to UPrimitiveComponent, which replaces the confusing bUseMaxLODAsImposter & bBatchImpostersAsInstances.
		HLODBatchingPolicy,

		// Serialize scene components static bounds
		SerializeSceneComponentStaticBounds,

		// Add the long range attachment tethers to the cloth asset to avoid a large hitch during the cloth's initialization.
		ChaosClothAddTethersToCachedData,

		// Always serialize the actor label in cooked builds
		SerializeActorLabelInCookedBuilds,

		// Changed world partition HLODs cells from FSotObjectPath to FName
		ConvertWorldPartitionHLODsCellsToName,

		// Re-calculate the long range attachment to prevent kinematic tethers.
		ChaosClothRemoveKinematicTethers,

		// Serializes the Morph Target render data for cooked platforms and the DDC
		SerializeSkeletalMeshMorphTargetRenderData,

		// Strip the Morph Target source data for cooked builds
		StripMorphTargetSourceDataForCookedBuilds,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

	FUE5PrivateFrostyStreamObjectVersion() = delete;
};
