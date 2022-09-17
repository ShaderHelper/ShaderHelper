// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AllowWindowsPLatformTypes.h: Defines for allowing the use of Windows types.
=============================================================================*/

#include "HoloLens/WindowsHWrapper.h"


#ifndef WINDOWS_PLATFORM_TYPES_GUARD
	#define WINDOWS_PLATFORM_TYPES_GUARD
#else
	#error Nesting AllowWindowsPlatformTypes.h is not allowed!
#endif

#define INT ::INT
#define UINT ::UINT
#define DWORD ::DWORD
#define FLOAT ::FLOAT

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
