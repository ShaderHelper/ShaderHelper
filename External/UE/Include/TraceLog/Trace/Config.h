// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"

#if !defined(UE_TRACE_ENABLED)
#	if !UE_BUILD_SHIPPING && !IS_PROGRAM
#		if PLATFORM_WINDOWS || PLATFORM_UNIX || PLATFORM_APPLE || PLATFORM_SWITCH || PLATFORM_ANDROID || PLATFORM_HOLOLENS
#			define UE_TRACE_ENABLED	1
#		endif
#	endif
#endif

#if !defined(UE_TRACE_ENABLED)
#	define UE_TRACE_ENABLED 0
#endif
