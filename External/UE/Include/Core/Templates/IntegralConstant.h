// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

/**
 * Defines a value metafunction of the given Value.
 */
template <typename T, T Val>
struct TIntegralConstant
{
	static constexpr T Value = Val;
};
