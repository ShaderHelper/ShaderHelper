// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ClangPlatformMath.h: Clang intrinsic implementations of some platform Math functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMath.h"

/**
 * Clang implementation of math functions
 **/
struct FClangPlatformMath : public FGenericPlatformMath
{
	/**
	 * Counts the number of leading zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of leading zeros for
	 *
	 * @return the number of zeros before the first "on" bit
	 */
	static FORCEINLINE uint8 CountLeadingZeros8(uint8 Value)
	{
		return uint8(__builtin_clz((uint32(Value) << 1) | 1) - 23);
	}

	/**
	 * Counts the number of leading zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of leading zeros for
	 *
	 * @return the number of zeros before the first "on" bit
	 */
	static FORCEINLINE uint32 CountLeadingZeros(uint32 Value)
	{
		return __builtin_clzll((uint64(Value) << 1) | 1) - 31;
	}

	/**
	 * Counts the number of leading zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of leading zeros for
	 *
	 * @return the number of zeros before the first "on" bit
	 */
	static FORCEINLINE uint64 CountLeadingZeros64(uint64 Value)
	{
		if (Value == 0)
		{
			return 64;
		}

		return __builtin_clzll(Value);
	}

	/**
	 * Counts the number of trailing zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of trailing zeros for
	 *
	 * @return the number of zeros after the last "on" bit
	 */
	static FORCEINLINE uint32 CountTrailingZeros(uint32 Value)
	{
		if (Value == 0)
		{
			return 32;
		}
	
		return (uint32)__builtin_ctz(Value);
	}
	
	/**
	 * Counts the number of trailing zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of trailing zeros for
	 *
	 * @return the number of zeros after the last "on" bit
	 */
	static FORCEINLINE uint64 CountTrailingZeros64(uint64 Value)
	{
		if (Value == 0)
		{
			return 64;
		}
	
		return (uint64)__builtin_ctzll(Value);
	}

	static FORCEINLINE uint32 FloorLog2(uint32 Value)
	{
		int32 Mask = -int32(Value != 0);
		return (31 - __builtin_clz(Value)) & Mask;
	}

	static FORCEINLINE uint64 FloorLog2_64(uint64 Value)
	{
		int64 Mask = -int64(Value != 0);
		return (63 - __builtin_clzll(Value)) & Mask;
	}
};
