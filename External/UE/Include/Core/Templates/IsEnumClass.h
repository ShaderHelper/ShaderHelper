// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/AndOrNot.h"

namespace UE::Core::Private::IsEnumClass
{
	template <typename T>
	struct TIsEnumConvertibleToInt
	{
		static char (&Resolve(int))[2];
		static char Resolve(...);

		enum { Value = sizeof(Resolve(T())) - 1 };
	};
}

/**
 * Traits class which tests if a type is arithmetic.
 */
template <typename T>
struct TIsEnumClass
{ 
	enum { Value = TAndValue<__is_enum(T), TNot<UE::Core::Private::IsEnumClass::TIsEnumConvertibleToInt<T>>>::Value };
};
