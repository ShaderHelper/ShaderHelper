// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in the //Fortnite/Main stream
struct CORE_API FFortniteReleaseBranchCustomObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		
		// Custom 14.10 File Object Version
		DisableLevelset_v14_10 ,
		
		// Add the long range attachment tethers to the cloth asset to avoid a large hitch during the cloth's initialization.
		ChaosClothAddTethersToCachedData,

		// Chaos::TKinematicTarget no longer stores a full transform, only position/rotation.
		ChaosKinematicTargetRemoveScale,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FFortniteReleaseBranchCustomObjectVersion() {}
};
