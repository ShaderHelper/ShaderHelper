// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in Dev-VR stream
struct CORE_API FVRObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		// Change UMotionControllerComponent from using EControllerHand to FName for motion source
		UseFNameInsteadOfEControllerHandForMotionSource,

		// Change how ARSessionConfig stores plane detection configuration from bitmask to bools
		UseBoolsForARSessionConfigPlaneDetectionConfiguration,

		// Change how UStereoLayerComponent stores additional properties for non-quad layer types
		UseSubobjectForStereoLayerShapeProperties,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FVRObjectVersion() {}
};
