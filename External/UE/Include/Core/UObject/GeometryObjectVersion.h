// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in Private-Geometry stream
struct CORE_API FGeometryObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		// Compress Geometry Cache Flipbooks to save disk space
		CompressGeometryCache,
		// Fix for serializing in Mesh vertices for new DynamicMeshVertex layout
		DynamicMeshVertexLayoutChange,
		//Added support for explicit motion vectors
		ExplicitMotionVectors,
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FGeometryObjectVersion() {}
};
