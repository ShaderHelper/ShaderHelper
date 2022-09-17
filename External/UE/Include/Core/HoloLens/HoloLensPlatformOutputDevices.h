// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	HoloLensOutputDevices.h: Windows platform OutputDevices functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformOutputDevices.h"

struct CORE_API FHoloLensOutputDevices : public FGenericPlatformOutputDevices
{
	static FOutputDevice*			GetEventLog();
};

typedef FHoloLensOutputDevices FPlatformOutputDevices;
