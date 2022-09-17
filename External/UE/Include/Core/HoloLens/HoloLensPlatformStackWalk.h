// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	HoloLensStackWalk.h: HoloLens platform stack walk functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformStackWalk.h"

struct CORE_API FHololensPlatformStackWalk
	: public FGenericPlatformStackWalk
{
	static TArray<FProgramCounterSymbolInfo> GetStack(int32 IgnoreCount, int32 MaxDepth = 100, void* Context = nullptr);
};

typedef FHololensPlatformStackWalk FPlatformStackWalk;
