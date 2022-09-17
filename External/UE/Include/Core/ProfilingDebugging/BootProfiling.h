// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

// TODO: Consider moving boot timing stuff from CoreGlobals.h here in UE5 - involves adding a new include to 50+ files

extern CORE_API double GEnginePreInitPreStartupScreenEndTime;
extern CORE_API double GEnginePreInitPostStartupScreenEndTime;
extern CORE_API double GEngineInitEndTime;

struct FBootProfiling
{
	static CORE_API double GetBootDuration();
	static CORE_API double GetPreInitPreStartupScreenDuration();
	static CORE_API double GetPreInitPostStartupScreenDuration();
	static CORE_API double GetEngineInitDuration();
};