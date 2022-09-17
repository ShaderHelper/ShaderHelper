// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MacPlatformMath.h: Mac platform Math functions
==============================================================================================*/

#pragma once
#include "Clang/ClangPlatformMath.h"
#include "Mac/MacSystemIncludes.h"


#if PLATFORM_MAC_X86
    #include <smmintrin.h>
    #include "Math/UnrealPlatformMathSSE4.h"

    /*
    * Mac implementation of the Math OS functions
    **/
    struct FMacPlatformMath : public TUnrealPlatformMathSSE4Base<FClangPlatformMath>
    {
    #if PLATFORM_ENABLE_POPCNT_INTRINSIC
        /**
         * Use the SSE instruction to count bits
         */
        static FORCEINLINE int32 CountBits(uint64 Bits)
        {
            return __builtin_popcountll(Bits);
        }
    #endif

        static FORCEINLINE bool IsNaN( float A ) { return isnan(A) != 0; }
		static FORCEINLINE bool IsNaN(double A) { return isnan(A) != 0; }
		static FORCEINLINE bool IsFinite(float A) { return isfinite(A); }
		static FORCEINLINE bool IsFinite(double A) { return isfinite(A); }
	};

    typedef FMacPlatformMath FPlatformMath;
#else
    typedef FClangPlatformMath FPlatformMath;
#endif

