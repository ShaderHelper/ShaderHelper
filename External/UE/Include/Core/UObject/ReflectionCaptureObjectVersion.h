// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

// Custom serialization version for changes made for a private stream
struct CORE_API FReflectionCaptureObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,

		// Allows uncompressed reflection captures for cooked builds
		MoveReflectionCaptureDataToMapBuildData,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FReflectionCaptureObjectVersion() {}
};
