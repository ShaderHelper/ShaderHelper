// Copyright Epic Games, Inc. All Rights Reserved.

#if PLATFORM_WINDOWS
	#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/HideWindowsPlatformTypes.h"
#else
	#include "Microsoft/HideMicrosoftPlatformTypesPrivate.h"
#endif
