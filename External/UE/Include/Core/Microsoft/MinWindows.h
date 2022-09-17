// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#if PLATFORM_WINDOWS
	#include "Windows/MinWindows.h"
#elif PLATFORM_HOLOLENS
	#include "HoloLens/MinWindows.h"
#else
	#include "Microsoft/MinWindowsPrivate.h"
#endif
