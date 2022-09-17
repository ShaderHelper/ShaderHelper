// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"

// Custom serialization version for changes made in Dev-Editor stream
struct CORE_API FMediaFrameworkObjectVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		// Serialize GUIDs instead of plain names for MediaSource platform/player map
		SerializeGUIDsInMediaSourceInsteadOfPlainNames,
		// Serialize GUIDs instead of plain names for PlatformMediaSource platform/mediasource map
		SerializeGUIDsInPlatformMediaSourceInsteadOfPlainNames,
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FMediaFrameworkObjectVersion() {}
};
