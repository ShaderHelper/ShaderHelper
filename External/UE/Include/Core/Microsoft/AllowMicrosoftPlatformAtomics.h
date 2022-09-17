// Copyright Epic Games, Inc. All Rights Reserved.

#if PLATFORM_WINDOWS
	#include "Windows/AllowWindowsPlatformAtomics.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/AllowWindowsPlatformAtomics.h"
#else
	#include "Microsoft/AllowMicrosoftPlatformAtomicsPrivate.h"
#endif
