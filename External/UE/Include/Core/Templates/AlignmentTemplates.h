// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/IsIntegral.h"
#include "Templates/IsPointer.h"

/**
 * Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, must be a power of two.
 *
 * @return The value aligned up to the specified alignment.
 */
template <typename T>
FORCEINLINE constexpr T Align(T Val, uint64 Alignment)
{
	static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "Align expects an integer or pointer type");

	return (T)(((uint64)Val + Alignment - 1) & ~(Alignment - 1));
}

/**
 * Aligns a value to the nearest lower multiple of 'Alignment', which must be a power of two.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, must be a power of two.
 *
 * @return The value aligned down to the specified alignment.
 */
template <typename T>
FORCEINLINE constexpr T AlignDown(T Val, uint64 Alignment)
{
	static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "AlignDown expects an integer or pointer type");

	return (T)(((uint64)Val) & ~(Alignment - 1));
}

/**
 * Checks if a pointer is aligned to the specified alignment.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, must be a power of two.
 *
 * @return true if the pointer is aligned to the specified alignment, false otherwise.
 */
template <typename T>
FORCEINLINE constexpr bool IsAligned(T Val, uint64 Alignment)
{
	static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "IsAligned expects an integer or pointer type");

	return !((uint64)Val & (Alignment - 1));
}

/**
 * Aligns a value to the nearest higher multiple of 'Alignment'.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, can be any arbitrary value.
 *
 * @return The value aligned up to the specified alignment.
 */
template <typename T>
FORCEINLINE constexpr T AlignArbitrary(T Val, uint64 Alignment)
{
	static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "AlignArbitrary expects an integer or pointer type");

	return (T)((((uint64)Val + Alignment - 1) / Alignment) * Alignment);
}
