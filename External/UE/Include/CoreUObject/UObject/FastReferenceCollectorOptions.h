// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/*===================================================================================
	FastReferenceCollectorOptions.h: Global TFastReferenceCollector enums and defines
=====================================================================================*/

enum class EFastReferenceCollectorOptions : uint32
{
	None = 0,
	Parallel = 1 << 0,
	AutogenerateTokenStream = 1 << 1,
	ProcessNoOpTokens = 1 << 2,
	WithClusters = 1 << 3, 
	ProcessWeakReferences = 1 << 4,
	WithPendingKill = 1 << 5,
};
ENUM_CLASS_FLAGS(EFastReferenceCollectorOptions);
