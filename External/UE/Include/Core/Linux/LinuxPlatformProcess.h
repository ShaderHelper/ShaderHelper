// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	LinuxPlatformProcess.h: Linux platform Process functions
==============================================================================================*/

#pragma once

#include "Unix/UnixPlatformProcess.h"

/**
 * Linux implementation of the Process OS functions
 */
struct CORE_API FLinuxPlatformProcess : public FUnixPlatformProcess
{
	static const TCHAR* BaseDir();
	static const TCHAR* GetBinariesSubdirectory();
};

typedef FLinuxPlatformProcess FPlatformProcess;
