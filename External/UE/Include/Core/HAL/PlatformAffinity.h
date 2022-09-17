// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

#include COMPILED_PLATFORM_HEADER(PlatformAffinity.h)

struct FThreadAffinity 
{ 
	uint64 ThreadAffinityMask = FPlatformAffinity::GetNoAffinityMask(); 
	uint16 ProcessorGroup = 0; 
};