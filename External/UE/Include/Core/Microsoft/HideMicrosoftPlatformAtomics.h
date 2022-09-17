// Copyright Epic Games, Inc. All Rights Reserved.

#if PLATFORM_WINDOWS
	#include "Windows/HideWindowsPlatformAtomics.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/HideWindowsPlatformAtomics.h"
#else
	#include "Microsoft/HideMicrosoftPlatformAtomicsPrivate.h"
#endif
