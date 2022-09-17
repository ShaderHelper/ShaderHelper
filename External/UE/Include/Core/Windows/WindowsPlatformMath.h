// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if !PLATFORM_WINDOWS
	#error this code should only be included on Windows
#endif

#include <intrin.h>
#include <smmintrin.h>
#include "Microsoft/MicrosoftPlatformMath.h"

typedef FMicrosoftPlatformMathBase FWindowsPlatformMath;
typedef FWindowsPlatformMath FPlatformMath;
