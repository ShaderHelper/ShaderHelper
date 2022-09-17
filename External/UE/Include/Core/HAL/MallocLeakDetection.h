// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocLeakDetection.h: Helper class to track memory allocations
=============================================================================*/

#pragma once

#include "CoreTypes.h"
#include "HAL/MemoryBase.h"
#include "HAL/UnrealMemory.h"
#include "Containers/Array.h"
#include "Misc/Crc.h"
#include "Containers/UnrealString.h"
#include "Containers/Set.h"
#include "Containers/Map.h"
#include "Misc/ScopeLock.h"
#include "HAL/ThreadSafeBool.h"

#ifndef MALLOC_LEAKDETECTION
	#define MALLOC_LEAKDETECTION 0
#endif

/**
 *	Options that can be supplied when calling FMallocLeakDetection::DumpOpenCallstacks
 */
struct FMallocLeakReportOptions
{
	enum class ESortOption
	{
		SortSize,
		SortRate,
		SortHash
	};

	FMallocLeakReportOptions()
	{
		FMemory::Memzero(this, sizeof(FMallocLeakReportOptions));
	}

	/** If >0 only report allocations greater than this size */
	uint32			SizeFilter;

	/** If >0 only report allocations at a greater bytes/frame than this */
	float			RateFilter;

	/** Restrict report to allocations that have no history of being deleted */
	bool			OnlyNonDeleters;

	/** Only show allocations after this frame */
	uint32			FrameStart;

	/** Only show allocations from before this frame */
	uint32			FrameEnd;

	/** Sort allocations by this (default - size) */
	ESortOption		SortBy;
};

#if MALLOC_LEAKDETECTION

/**
 * Maintains a list of all pointers to currently allocated memory.
 */
class CORE_API FMallocLeakDetection
{
	struct FCallstackTrack
	{
		FCallstackTrack()
		{
			FMemory::Memzero(this, sizeof(FCallstackTrack));
		}
		static constexpr int32 Depth = 32;
		uint64 CallStack[Depth];
		uint32 FirstFrame;
		uint32 LastFrame;
		uint64 Size;
		uint32 Count;
		uint32 CachedHash;

		// least square line fit stuff
		uint32 NumCheckPoints;
		float SumOfFramesNumbers;
		float SumOfFramesNumbersSquared;
		float SumOfMemory;
		float SumOfMemoryTimesFrameNumber;

		// least square line results
		float Baseline;
		float BytesPerFrame;

		bool operator==(const FCallstackTrack& Other) const
		{
			bool bEqual = true;
			for (int32 i = 0; i < Depth; ++i)
			{
				if (CallStack[i] != Other.CallStack[i])
				{
					bEqual = false;
					break;
				}
			}
			return bEqual;
		}

		bool operator!=(const FCallstackTrack& Other) const
		{
			return !(*this == Other);
		}

		void GetLinearFit();
		
		uint32 GetHash() 
		{
			CachedHash = FCrc::MemCrc32(CallStack, sizeof(CallStack), 0);
			return CachedHash;
		}
	};

private:

	FMallocLeakDetection();
	~FMallocLeakDetection();

	/** Track a callstack */

	/** Stop tracking a callstack */
	void AddCallstack(FCallstackTrack& Callstack);
	void RemoveCallstack(FCallstackTrack& Callstack);

	/** List of all currently allocated pointers */
	TMap<void*, FCallstackTrack> OpenPointers;

	/** List of all unique callstacks with allocated memory */
	TMap<uint32, FCallstackTrack> UniqueCallstacks;

	/** Set of callstacks that are known to delete memory (not reset on ClearData()) */
	TSet<uint32>	KnownDeleters;

	/** Set of callstacks that are known to resize memory (not reset on ClearData()) */
	TSet<uint32>	KnownTrimmers;

	/** Contexts that are associated with allocations */
	TMap<void*, FString>		PointerContexts;

	/** Stack of contexts */
	struct FContextString { TCHAR Buffer[64]; };
	TArray<FContextString>	Contexts;
		
	/** Critical section for mutating internal data */
	FCriticalSection AllocatedPointersCritical;	

	/** Set during mutating operations to prevent internal allocations from recursing */
	bool	bRecursive;

	/** Is allocation capture enabled? */
	bool	bCaptureAllocs;

	/** Minimal size to capture? */
	int32	MinAllocationSize;

	/** Size of all tracked callstacks */
	SIZE_T	TotalTracked;

	/** How long since we compacted things? */
	int32	AllocsWithoutCompact;

public:	

	static FMallocLeakDetection& Get();
	static void HandleMallocLeakCommand(const TArray< FString >& Args);

	/** Enable/disable collection of allocation with an optional filter on allocation size */
	void SetAllocationCollection(bool bEnabled, int32 Size = 0);

	/** Returns state of allocation collection */
	bool IsAllocationCollectionEnabled(void) const { return bCaptureAllocs; }

	/** Clear currently accumulated data */
	void ClearData();

	/** Dumps currently open callstacks */
	int32 DumpOpenCallstacks(const TCHAR* FileName, const FMallocLeakReportOptions& Options = FMallocLeakReportOptions());

	/** Perform a linear fit checkpoint of all open callstacks */
	void CheckpointLinearFit();

	/** Handles new allocated pointer */
	void Malloc(void* Ptr, SIZE_T Size);

	/** Handles reallocation */
	void Realloc(void* OldPtr, SIZE_T OldSize, void* NewPtr, SIZE_T NewSize);

	/** Removes allocated pointer from list */
	void Free(void* Ptr);	

	/** Disabled allocation tracking for this thread, @see MALLOCLEAK_IGNORE_SCOPE and FMallocLeakDetectionIgnoreScope. */
	void SetDisabledForThisThread(const bool Disabled);

	/** Returns true of allocation tracking for this thread is  */
	bool IsDisabledForThisThread() const;

	/** Pushes context that will be associated with allocations. All open contexts will be displayed alongside
	callstacks in a report.  */
	void PushContext(const FString& Context)
	{
		this->PushContext(*Context);
	}
	void PushContext(const TCHAR* Context);

	/** Pops a context from the above */
	void PopContext();

	/** Returns */
	void GetOpenCallstacks(TArray<uint32>& OutCallstacks, SIZE_T& OutTotalSize, const FMallocLeakReportOptions& Options = FMallocLeakReportOptions());
};

/**
 *	Helper class that can be used to ignore allocations from a specific scope for leak detection.
 *	Use this carefully and only if you know that a portion of code is throwing up either false
 *	positives or can be ignored. (e.g., one example is the FName table which never shrinks
 *	and eventually reaches a max that is relatively inconsequential).
 */
class FMallocLeakDetectionIgnoreScope
{
public:
	FMallocLeakDetectionIgnoreScope()
	{
		FMallocLeakDetection::Get().SetDisabledForThisThread(true);
	}

	~FMallocLeakDetectionIgnoreScope()
	{
		FMallocLeakDetection::Get().SetDisabledForThisThread(false);
	}

	// Non-copyable
	FMallocLeakDetectionIgnoreScope(const FMallocLeakDetectionIgnoreScope&) = delete;
	FMallocLeakDetectionIgnoreScope& operator=(const FMallocLeakDetectionIgnoreScope&) = delete;
};

class FMallocLeakScopedContext
{
public:
	template <typename ArgType>
	explicit FMallocLeakScopedContext(ArgType&& Context)
	{
		FMallocLeakDetection::Get().PushContext(Forward<ArgType>(Context));
	}

	~FMallocLeakScopedContext()
	{
		FMallocLeakDetection::Get().PopContext();
	}

	// Non-copyable
	FMallocLeakScopedContext(const FMallocLeakScopedContext&) = delete;
	FMallocLeakScopedContext& operator=(const FMallocLeakScopedContext&) = delete;
};

#define MALLOCLEAK_IGNORE_SCOPE() \
	FMallocLeakDetectionIgnoreScope ANONYMOUS_VARIABLE(DetectionShouldIgnoreScope)

#define MALLOCLEAK_SCOPED_CONTEXT(Context) \
	FMallocLeakScopedContext ANONYMOUS_VARIABLE(ScopedContext)(Context)

#else // MALLOC_LEAKDETECTION 0

#define MALLOCLEAK_IGNORE_SCOPE()
#define MALLOCLEAK_SCOPED_CONTEXT(Context)

#endif // MALLOC_LEAKDETECTION

