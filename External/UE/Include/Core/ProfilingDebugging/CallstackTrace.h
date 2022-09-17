// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "TraceLog/Public/Trace/Config.h"

////////////////////////////////////////////////////////////////////////////////
#if !defined(UE_CALLSTACK_TRACE_ENABLED)
	#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
		#if PLATFORM_WINDOWS 
			#define UE_CALLSTACK_TRACE_ENABLED 1
		#endif
	#endif
#endif

#if !defined(UE_CALLSTACK_TRACE_ENABLED)
	#define UE_CALLSTACK_TRACE_ENABLED 0
#endif

////////////////////////////////////////////////////////////////////////////////
#if UE_CALLSTACK_TRACE_ENABLED

/**
 * Creates callstack tracing.
 * @param Malloc Allocator instance to use.
 */
void CallstackTrace_Create(class FMalloc* Malloc);

/**
 * Initializes callstack tracing. On some platforms this has to be delayed due to initialization order.
 */
void CallstackTrace_Initialize();

/**
 * Capture the current callstack, and trace the definition if it has not already been encountered. The returned value
 * can be used in trace events and be resolved in analysis.
 * @return Unique id identifying the current callstack.
 */
CORE_API uint32 CallstackTrace_GetCurrentId();

#else // UE_CALLSTACK_TRACE_ENABLED

inline void CallstackTrace_Create(class FMalloc* Malloc) {}
inline void CallstackTrace_Initialize() {}
inline uint32 CallstackTrace_GetCurrentId() { return 0; }

#endif // UE_CALLSTACK_TRACE_ENABLED
