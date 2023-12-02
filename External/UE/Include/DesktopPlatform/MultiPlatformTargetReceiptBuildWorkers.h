// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TargetReceiptBuildWorker.h"
#include "Templates/TypeCompatibleBytes.h"

/**
 * Globally registers a UE::DerivedData::IBuildWorkerFactory instance for each platform that build workers can be supported.
 * Users should include a $(Platform) variable in their file path to indicate that this worker receipt can exist for any platform.
 * If the receipt path does not include a $(Platform) variable, then no attempt will be made to find variations of it for other platforms.
*/
class DESKTOPPLATFORM_API FMultiPlatformTargetReceiptBuildWorkers
{
public:
	FMultiPlatformTargetReceiptBuildWorkers(const TCHAR* TargetReceiptFilePath);
	~FMultiPlatformTargetReceiptBuildWorkers();

private:
	enum ESupportedPlatform : uint8
	{
		SupportedPlatform_Win64 = 0,
		SupportedPlatform_Mac,
		SupportedPlatform_Linux,

		SupportedPlatform_MAX
	};
	TTypeCompatibleBytes<FTargetReceiptBuildWorker> PlatformSpecificWorkerFactories[SupportedPlatform_MAX];
	bool bAllPlatformsInitialized;
};
