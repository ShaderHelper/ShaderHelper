// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealTemplate.h"

/**
 * Binary predicate class for performing equality comparisons.
 *
 * Uses operator== when available.
 *
 * See: https://en.cppreference.com/w/cpp/utility/functional/equal_to
 */
template <typename T = void>
struct TEqualTo
{
	constexpr auto operator()(const T& Lhs, const T& Rhs) const -> decltype(Lhs == Rhs)
	{
		return Lhs == Rhs;
	}
};

template <>
struct TEqualTo<void>
{
	template <typename T, typename U>
	constexpr auto operator()(T&& Lhs, U&& Rhs) const -> decltype(Forward<T>(Lhs) == Forward<U>(Rhs))
	{
		return Forward<T>(Lhs) == Forward<U>(Rhs);
	}
};
