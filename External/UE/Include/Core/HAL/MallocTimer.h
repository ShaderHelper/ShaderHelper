// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreTypes.h"
#include "HAL/CriticalSection.h"
#include "HAL/PlatformTime.h"

#ifndef UE_TIME_VIRTUALMALLOC
#define UE_TIME_VIRTUALMALLOC 0
#endif

struct CORE_API FScopedVirtualMallocTimer
{
	enum IndexType : int32 
	{
		Reserve,
		Commit,
		Combined,
		DeCommit,
		Free,

		Max
	};

	enum PlatormIndexType : int32
	{
		OrdinaryCPU, // General memory
		GPU_WriteCombine, // XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_WRITECOMBINE and XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE
		GPU_Cacheable, // XALLOC_MEMTYPE_GRAPHICS_CACHEABLE
		GPU_WriteCombineRenderTarget, // Similar to GPU_WriteCombine, but with 4MB pages and up to 128K alignment, no small block allocator
		PlatormIndexTypeMax
	};

#if UE_TIME_VIRTUALMALLOC
	static uint64 GTotalCycles[IndexType::Max][PlatormIndexType::PlatormIndexTypeMax];
	static uint64 GTotalCounts[IndexType::Max][PlatormIndexType::PlatormIndexTypeMax];

	int32 Index;
	int32 PlatformTypeIndex;
	uint64 Cycles;

	FORCEINLINE FScopedVirtualMallocTimer(int32 InIndex = 0, int32 InPlatformTypeIndex = 0)
		: Index(InIndex)
		, PlatformTypeIndex(InPlatformTypeIndex)
		, Cycles(FPlatformTime::Cycles64())
	{
		FPlatformAtomics::InterlockedIncrement((volatile int64*)&GTotalCounts[InIndex][InPlatformTypeIndex]);
	}
	FORCEINLINE ~FScopedVirtualMallocTimer()
	{
		uint64 Add = uint64(FPlatformTime::Cycles64() - Cycles);
		FPlatformAtomics::InterlockedAdd((volatile int64*)&GTotalCycles[Index][PlatformTypeIndex], Add);
	}

	static void UpdateStats();
#else	//UE_TIME_VIRTUALMALLOC
	FORCEINLINE FScopedVirtualMallocTimer(int32 InIndex = 0, int32 InPlatformTypeIndex = 0)
	{
	}
	FORCEINLINE ~FScopedVirtualMallocTimer()
	{
	}
	static FORCEINLINE void UpdateStats()
	{
	}
#endif	//UE_TIME_VIRTUALMALLOC
};
