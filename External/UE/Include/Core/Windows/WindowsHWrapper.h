// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

// #TODO: redirect to platform-agnostic version for the time being. Eventually this will become an error
#include "HAL/Platform.h"
#if !PLATFORM_WINDOWS && !PLATFORM_HOLOLENS
	#include "Microsoft/WindowsHWrapper.h"
#else

#include "CoreTypes.h"
#include "HAL/PlatformMemory.h"
#include "Windows/PreWindowsApi.h"
#ifndef STRICT
#define STRICT
#endif
#include "Windows/MinWindows.h"
#include "Windows/PostWindowsApi.h"

#endif //PLATFORM_*