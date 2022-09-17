// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	LinuxPlatformMisc.h: Linux platform misc functions
==============================================================================================*/

#pragma once

#include "Unix/UnixPlatformMisc.h"

/**
 * Linux implementation of the misc OS functions
 */

// A stub for now. Will allow to override any functions if needed
struct CORE_API FLinuxPlatformMisc : public FUnixPlatformMisc
{
};

typedef FLinuxPlatformMisc FPlatformMisc;
