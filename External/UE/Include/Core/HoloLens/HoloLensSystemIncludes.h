// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once


#include "CoreTypes.h"

#ifndef STRICT
#define STRICT
#endif

#include "HoloLens/HoloLensPlatformCompilerSetup.h"
#include "Windows/MinimalWindowsApi.h"

// Macro for releasing COM objects
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
THIRD_PARTY_INCLUDES_START

// SIMD intrinsics
#include <intrin.h>

#include <intsafe.h>
#include <strsafe.h>
#include <tchar.h>

#include <stdint.h>
#include <concrt.h>

THIRD_PARTY_INCLUDES_END

#ifndef OUT
#define OUT
#endif

#ifndef IN
#define IN
#endif
