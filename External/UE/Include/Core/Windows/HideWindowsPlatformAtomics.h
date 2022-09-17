// Copyright Epic Games, Inc. All Rights Reserved.

// #TODO: redirect to platform-agnostic version for the time being. Eventually this will become an error
#include "HAL/Platform.h"
#if !PLATFORM_WINDOWS && !PLATFORM_HOLOLENS
	#include "Microsoft/HideMicrosoftPlatformAtomics.h"
#else

#ifdef WINDOWS_PLATFORM_ATOMICS_GUARD
	#undef WINDOWS_PLATFORM_ATOMICS_GUARD
#else
	#error Mismatched HideWindowsPlatformAtomics.h detected.
#endif

#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedAdd
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedCompareExchange
#undef InterlockedCompareExchangePointer
#undef InterlockedExchange64
#undef InterlockedExchangeAdd64
#undef InterlockedCompareExchange64
#undef InterlockedIncrement64
#undef InterlockedDecrement64
#undef InterlockedAnd
#undef InterlockedOr
#undef InterlockedXor


#endif //PLATFORM_*
