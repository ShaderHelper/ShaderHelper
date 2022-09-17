// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/AreTypesEqual.h"
#include "Templates/CopyQualifiersFromTo.h"

/**
 * Tests if qualifiers are lost between one type and another, e.g.:
 *
 * TLosesQualifiersFromTo<const    T1,                T2>::Value == true
 * TLosesQualifiersFromTo<volatile T1, const volatile T2>::Value == false
 */
template <typename From, typename To>
struct TLosesQualifiersFromTo
{
	enum { Value = !TAreTypesEqual<typename TCopyQualifiersFromTo<From, To>::Type, To>::Value };
};
