// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if !PLATFORM_WINDOWS && !PLATFORM_HOLOLENS
	#error this file is for PLATFORM_WINDOWS or PLATFORM_HOLOLENS only
#endif

// redirect to new file
#include "Microsoft/MinimalWindowsApi.h"
