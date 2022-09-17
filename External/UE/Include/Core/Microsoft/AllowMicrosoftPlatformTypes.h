// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
	#include "Windows/AllowWindowsPlatformTypes.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/AllowWindowsPlatformTypes.h"
#else
	#include "Microsoft/AllowMicrosoftPlatformTypesPrivate.h"
#endif
