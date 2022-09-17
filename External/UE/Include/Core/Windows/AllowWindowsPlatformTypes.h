// Copyright Epic Games, Inc. All Rights Reserved.

// #TODO: redirect to platform-agnostic version for the time being. Eventually this will become an error
#include "HAL/Platform.h"
#if !PLATFORM_WINDOWS && !PLATFORM_HOLOLENS
	#include "Microsoft/AllowMicrosoftPlatformTypes.h"
#else


#include "Windows/WindowsHWrapper.h"

#ifndef WINDOWS_PLATFORM_TYPES_GUARD
	#define WINDOWS_PLATFORM_TYPES_GUARD
#else
	#error Nesting AllowWindowsPlatformTypes.h is not allowed!
#endif

#pragma warning( push )
#pragma warning( disable : 4459 )

#define INT ::INT
#define UINT ::UINT
#define DWORD ::DWORD
#define FLOAT ::FLOAT

#define TRUE 1
#define FALSE 0

#endif //PLATFORM_*
