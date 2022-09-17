// Copyright Epic Games, Inc. All Rights Reserved.

#if PLATFORM_WINDOWS
	#include "Windows/PostWindowsApi.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/PostWindowsApi.h"
#else
	#include "Microsoft/PostWindowsApiPrivate.h"
#endif
