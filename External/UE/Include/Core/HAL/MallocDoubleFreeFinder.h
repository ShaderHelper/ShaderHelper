// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/MallocCallstackHandler.h"

class CORE_API FMallocDoubleFreeFinder final : public FMallocCallstackHandler
{
public:
	FMallocDoubleFreeFinder(FMalloc* InMalloc);

	/**
	 * Handles any commands passed in on the command line
	 */
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/**
	 * If you get an allocation/memory error outside of the allocator you can call this directly
	 * It will dump a callstack of the last allocator free most likely to have caused the problem to the log, if you have symbols loaded
	 * Might be useful to pass an access violation ptr to this!
	 */
	void TrackSpecial(void* Ptr);

	virtual void Init();

	static FMalloc* OverrideIfEnabled(FMalloc*InUsedAlloc);

protected:
	virtual void TrackMalloc(void* Ptr, uint32 Size, int32 CallStackIndex);
	virtual void TrackFree(void* Ptr, uint32 OldSize, int32 CallStackIndex);

	struct TrackedAllocationData
	{
		SIZE_T Size;
		int32 CallStackIndex;
		TrackedAllocationData() :
			Size(0),
			CallStackIndex(-1)
		{
		};
		TrackedAllocationData(SIZE_T InRequestedSize, int32 InCallStackIndex)
		{
			Size = InRequestedSize;
			CallStackIndex = InCallStackIndex;
		};
		~TrackedAllocationData()
		{
			Size = 0;
			CallStackIndex = -1;
		};
	};
	TMap<const void* const, TrackedAllocationData> TrackedCurrentAllocations;	// Pointer as a key to a call stack for all the current allocations we known about
	TMap<const void* const, TrackedAllocationData> TrackedFreeAllocations;		// Pointer as a key to a call stack for all allocations that have been freed
};

extern CORE_API FMallocDoubleFreeFinder* GMallocDoubleFreeFinder;
extern CORE_API bool GMallocDoubleFreeFinderEnabled;
