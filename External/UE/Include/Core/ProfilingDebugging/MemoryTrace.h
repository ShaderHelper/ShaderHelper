// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Trace/Trace.h"
#include "Misc/EnumClassFlags.h"

#if !defined(UE_MEMORY_TRACE_AVAILABLE)
#   define UE_MEMORY_TRACE_AVAILABLE 0
#endif

#if !defined(UE_MEMORY_TRACE_LATE_INIT)
#	define UE_MEMORY_TRACE_LATE_INIT 0
#endif

#if !defined(UE_MEMORY_TRACE_ENABLED) && UE_TRACE_ENABLED
#	if UE_MEMORY_TRACE_AVAILABLE
#		if !PLATFORM_USES_FIXED_GMalloc_CLASS && PLATFORM_64BITS
#			define UE_MEMORY_TRACE_ENABLED !UE_BUILD_SHIPPING
#		endif
#	endif
#endif

#if !defined(UE_MEMORY_TRACE_ENABLED)
#	define UE_MEMORY_TRACE_ENABLED 0
#endif

////////////////////////////////////////////////////////////////////////////////
typedef uint32 HeapId;

////////////////////////////////////////////////////////////////////////////////
enum EMemoryTraceRootHeap : uint8
{
	SystemMemory, // RAM
	VideoMemory, // VRAM
	EndHardcoded = VideoMemory,
	EndReserved = 15
};

////////////////////////////////////////////////////////////////////////////////
// These values are traced. Do not modify existing values in order to maintain
// compatibility.
enum class EMemoryTraceHeapFlags : uint16
{
	None = 0,
	Root = 1 << 0,
	NeverFrees = 1 << 1, // The heap doesn't free (e.g. linear allocator)
};
ENUM_CLASS_FLAGS(EMemoryTraceHeapFlags);

////////////////////////////////////////////////////////////////////////////////
// These values are traced. Do not modify existing values in order to maintain
// compatibility.
enum class EMemoryTraceHeapAllocationFlags : uint8
{
	None = 0,
	Heap = 1 << 0, // Is a heap, can be used to unmark alloc as heap.
};
ENUM_CLASS_FLAGS(EMemoryTraceHeapAllocationFlags);

#if UE_MEMORY_TRACE_ENABLED

////////////////////////////////////////////////////////////////////////////////
CORE_API UE_TRACE_CHANNEL_EXTERN(MemAllocChannel);

////////////////////////////////////////////////////////////////////////////////
class FMalloc* MemoryTrace_Create(class FMalloc* InMalloc);
void MemoryTrace_Initialize();

#if UE_MEMORY_TRACE_LATE_INIT
void MemoryTrace_InitializeLate();
#else
static void MemoryTrace_InitializeLate() {}
#endif // UE_MEMORY_TRACE_LATE_INIT

/**
 * Register a new heap specification (name). Use the returned value when marking heaps.
 * @param ParentId Heap id of parent heap.
 * @param Name Descriptive name of the heap.
 * @param Flags Properties of this heap. See \ref EMemoryTraceHeapFlags
 * @return Heap id to use when allocating memory
 */
CORE_API HeapId MemoryTrace_HeapSpec(HeapId ParentId, const TCHAR* Name, EMemoryTraceHeapFlags Flags = EMemoryTraceHeapFlags::None);

/**
 * Register a new root heap specification (name). Use the returned value as parent to other heaps.
 * @param Name Descriptive name of the root heap.
 * @param Flags Properties of the this root heap. See \ref EMemoryTraceHeapFlags
 * @return Heap id to use when allocating memory
 */
CORE_API HeapId MemoryTrace_RootHeapSpec(const TCHAR* Name, EMemoryTraceHeapFlags Flags = EMemoryTraceHeapFlags::None);

/**
 * Mark a traced allocation as being a heap.
 * @param Address Address of the allocation
 * @param Heap Heap id, see /ref MemoryTrace_HeapSpec. If no specific heap spec has been created the correct root heap needs to be given.
 * @param Flags Additional properties of the heap allocation. Note that \ref EMemoryTraceHeapAllocationFlags::Heap is implicit.
 */
CORE_API void MemoryTrace_MarkAllocAsHeap(uint64 Address, HeapId Heap, EMemoryTraceHeapAllocationFlags Flags = EMemoryTraceHeapAllocationFlags::None);

/**
 * Unmark an allocation as a heap. When an allocation that has previously been used as a heap is reused as a regular
 * allocation.
 * @param Address Address of the allocation
 * @param Heap Heap id
 */
CORE_API void MemoryTrace_UnmarkAllocAsHeap(uint64 Address, HeapId Heap);

/**
 * Trace an allocation event.
 * @param Address Address of allocation
 * @param Size Size of allocation
 * @param Alignment Alignment of the allocation
 * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
 */
CORE_API void MemoryTrace_Alloc(uint64 Address, uint64 Size, uint32 Alignment, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory);

/**
 * Trace a free event.
 * @param Address Address of the allocation being freed
 * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
 */
CORE_API void MemoryTrace_Free(uint64 Address, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory);

/**
 * Trace a free related to a reallocation event.
 * @param Address Address of the allocation being freed
 * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
 */
CORE_API void MemoryTrace_ReallocFree(uint64 Address, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory);

/** Trace an allocation related to a reallocation event.
 * @param Address Address of allocation
 * @param NewSize Size of allocation
 * @param Alignment Alignment of the allocation
 * @param RootHeap Which root heap this belongs to (system memory, video memory etc)
 */
CORE_API void MemoryTrace_ReallocAlloc(uint64 Address, uint64 NewSize, uint32 Alignment, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory);

#else

inline HeapId MemoryTrace_RootHeapSpec(const TCHAR* Name, EMemoryTraceHeapFlags Flags = EMemoryTraceHeapFlags::None) { return ~0; };
inline HeapId MemoryTrace_HeapSpec(HeapId ParentId, const TCHAR* Name, EMemoryTraceHeapFlags Flags = EMemoryTraceHeapFlags::None) { return ~0; }
inline void MemoryTrace_MarkAllocAsHeap(uint64 Address, HeapId Heap) {}
inline void MemoryTrace_UnmarkAllocAsHeap(uint64 Address, HeapId Heap) {}
inline void MemoryTrace_Alloc(uint64 Address, uint64 Size, uint32 Alignment, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory) {}
inline void MemoryTrace_Free(uint64 Address, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory) {}
inline void MemoryTrace_ReallocFree(uint64 Address, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory) {}
inline void MemoryTrace_ReallocAlloc(uint64 Address, uint64 NewSize, uint32 Alignment, HeapId RootHeap = EMemoryTraceRootHeap::SystemMemory) {}

#endif // UE_MEMORY_TRACE_ENABLED
