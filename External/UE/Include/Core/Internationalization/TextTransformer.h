// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"

/**
 * Utility for performing low-level localized transforms.
 * The implementation can be found in LegacyText.cpp and ICUText.cpp.
 */
class CORE_API FTextTransformer
{
public:
	static FString ToLower(const FString& InStr);
	static FString ToUpper(const FString& InStr);
};
