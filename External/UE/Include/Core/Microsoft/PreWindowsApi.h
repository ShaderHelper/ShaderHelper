// Copyright Epic Games, Inc. All Rights Reserved.

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
	#include "Windows/PreWindowsApi.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/PreWindowsApi.h"
#else
	#include "Microsoft/PreWindowsApiPrivate.h"
#endif
