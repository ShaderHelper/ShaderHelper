// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace StabilizeLocalizationKeys
{

#if WITH_EDITOR

/**
 * Convert any text that has been initialized from a raw string to have a deterministic key based on the given key root and the property name.
 * @note This function will recurse into arrays, sets, maps, and sub-structures.
 */
COREUOBJECT_API void StabilizeLocalizationKeysForProperty(FProperty* InProp, void* InPropData, const FString& InNamespace, const FString& InKeyRoot, const bool bAppendPropertyNameToKey = true);

/**
 * Walk through the struct and convert any text that has been initialized from a raw string to have a deterministic key based on the given key root and the property name.
 * @note This function will recurse into arrays, sets, maps, and sub-structures.
 */
COREUOBJECT_API void StabilizeLocalizationKeysForStruct(UStruct* InStruct, void* InStructData, const FString& InNamespace, const FString& InKeyRoot);

#endif	// WITH_EDITOR

}
