// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Math/UnrealPlatformMathSSE4.h"

/**
* Microsoft base implementation of the Math OS functions
**/
struct FMicrosoftPlatformMathBase : public TUnrealPlatformMathSSE4Base<FGenericPlatformMath>
{
#if PLATFORM_ENABLE_VECTORINTRINSICS
	static FORCEINLINE bool IsNaN( float A ) { return _isnan(A) != 0; }
	static FORCEINLINE bool IsNaN(double A) { return _isnan(A) != 0; }
	static FORCEINLINE bool IsFinite( float A ) { return _finite(A) != 0; }
	static FORCEINLINE bool IsFinite(double A) { return _finite(A) != 0; }

	#pragma intrinsic(_BitScanReverse)

	static FORCEINLINE uint32 FloorLog2(uint32 Value)
	{
		// Use BSR to return the log2 of the integer
		unsigned long Log2;
		if(_BitScanReverse(&Log2, Value) != 0)
		{
			return Log2;
		}

		return 0;
	}
	static FORCEINLINE uint8 CountLeadingZeros8(uint8 Value)
	{
		unsigned long Log2;
		_BitScanReverse(&Log2, (uint32(Value) << 1) | 1);
		return uint8(8 - Log2);
	}

	static FORCEINLINE uint32 CountTrailingZeros(uint32 Value)
	{
		if (Value == 0)
		{
			return 32;
		}
		unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 31
		_BitScanForward( &BitIndex, Value );	// Scans from LSB to MSB
		return BitIndex;
	}

	static FORCEINLINE uint32 CeilLogTwo( uint32 Arg )
	{
		int32 Bitmask = ((int32)(CountLeadingZeros(Arg) << 26)) >> 31;
		return (32 - CountLeadingZeros(Arg - 1)) & (~Bitmask);
	}

	static FORCEINLINE uint32 RoundUpToPowerOfTwo(uint32 Arg)
	{
		return 1 << CeilLogTwo(Arg);
	}

	static FORCEINLINE uint64 RoundUpToPowerOfTwo64(uint64 Arg)
	{
		return uint64(1) << CeilLogTwo64(Arg);
	}

#if PLATFORM_64BITS

	#pragma intrinsic(_BitScanReverse64)

	static FORCEINLINE uint64 FloorLog2_64(uint64 Value)
	{
		unsigned long Log2;
		long Mask = -long(_BitScanReverse64(&Log2, Value) != 0);
		return Log2 & Mask;
	}

	static FORCEINLINE uint64 CeilLogTwo64(uint64 Arg)
	{
		int64 Bitmask = ((int64)(CountLeadingZeros64(Arg) << 57)) >> 63;
		return (64 - CountLeadingZeros64(Arg - 1)) & (~Bitmask);
	}

	static FORCEINLINE uint64 CountLeadingZeros64(uint64 Value)
	{
		unsigned long Log2;
		long Mask = -long(_BitScanReverse64(&Log2, Value) != 0);
		return ((63 - Log2) & Mask) | (64 & ~Mask);
	}

	static FORCEINLINE uint64 CountTrailingZeros64(uint64 Value)
	{
		if (Value == 0)
		{
			return 64;
		}
		unsigned long BitIndex;	// 0-based, where the LSB is 0 and MSB is 31
		_BitScanForward64( &BitIndex, Value );	// Scans from LSB to MSB
		return BitIndex;
	}

	static FORCEINLINE uint32 CountLeadingZeros(uint32 Value)
	{
		unsigned long Log2;
		_BitScanReverse64(&Log2, (uint64(Value) << 1) | 1);
		return 32 - Log2;
	}

#else // 32-bit

	static FORCEINLINE uint32 CountLeadingZeros(uint32 Value)
	{
		unsigned long Log2;
		long Mask = -long(_BitScanReverse(&Log2, Value) != 0);
		return ((31 - Log2) & Mask) | (32 & ~Mask);
	}

#endif

#if PLATFORM_ENABLE_POPCNT_INTRINSIC
	static FORCEINLINE int32 CountBits(uint64 Bits)
	{
		return _mm_popcnt_u64(Bits);
	}
#endif

#endif
};
