// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	UnixPlatformOutputDevices.h: Unix platform OutputDevices functions
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "GenericPlatform/GenericPlatformOutputDevices.h"

struct CORE_API FUnixOutputDevices : public FGenericPlatformOutputDevices
{
	static void SetupOutputDevices();
	static FString GetAbsoluteLogFilename();
	static FOutputDevice* GetEventLog();
	static FOutputDeviceError* GetError();
};

typedef FUnixOutputDevices FPlatformOutputDevices;
