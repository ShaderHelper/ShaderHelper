// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/MallocCallstackHandler.h"

class CORE_API FMallocFrameProfiler final : public FMallocCallstackHandler
{
public:
	FMallocFrameProfiler(FMalloc* InMalloc);

	virtual void Init() override;
	
	/**
	 * Handles any commands passed in on the command line
	 */
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/** 
	* Called once per frame, gathers and sets all memory allocator statistics into the corresponding stats. MUST BE THREAD SAFE.
	*/
	virtual void UpdateStats() override;

	static FMalloc* OverrideIfEnabled(FMalloc*InUsedAlloc);

protected:
	virtual bool IsDisabled() override;

	virtual void TrackMalloc(void* Ptr, uint32 Size, int32 CallStackIndex);
	virtual void TrackFree(void* Ptr, uint32 OldSize, int32 CallStackIndex);
	virtual void TrackRealloc(void* OldPtr, void* NewPtr, uint32 NewSize, uint32 OldSize, int32 CallStackIndex);

protected:
	bool bEnabled;
	uint32 FrameCount;
	uint32 EntriesToOutput;

	struct FCallStackStats
	{
		int32 CallStackIndex = 0;
		int32 Mallocs = 0;
		int32 Frees = 0;
		int32 UsageCount = 0;
		int32 UniqueFrames = 0;
		int32 LastFrameSeen = 0;
	};

	TMap<void*, int32> TrackedCurrentAllocations;
	TArray<FCallStackStats> CallStackStatsArray;
};

extern CORE_API FMallocFrameProfiler* GMallocFrameProfiler;
extern CORE_API bool GMallocFrameProfilerEnabled;
