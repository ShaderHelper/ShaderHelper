// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
HoloLensTime.h: HoloLens platform Time functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformTime.h"
#include "HoloLens/HoloLensSystemIncludes.h"

/**
* HoloLens implementation of the Time OS functions
*/
struct CORE_API FHoloLensTime : public FGenericPlatformTime
{
	static double InitTiming();

	static FORCEINLINE double Seconds()
	{
		Windows::LARGE_INTEGER Cycles;
		Windows::QueryPerformanceCounter(&Cycles);
		// Add big number to make bugs apparent where return value is being passed to float
		return Cycles.QuadPart * GetSecondsPerCycle() + 16777216.0;
	}

	static FORCEINLINE uint32 Cycles()
	{
		Windows::LARGE_INTEGER Cycles;
		Windows::QueryPerformanceCounter(&Cycles);
		//@todo.HoloLens: QuadPart is a LONGLONG... can't use it here!
		//return Cycles.QuadPart;
		return Cycles.LowPart;
	}

	static FORCEINLINE uint64 Cycles64()
	{
		Windows::LARGE_INTEGER Cycles;
		Windows::QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
	}

	static void SystemTime(int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec);
	static void UtcTime(int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec);


	static bool UpdateCPUTime(float DeltaTime);
	static FCPUTime GetCPUTime();

protected:

	/** Percentage CPU utilization for the last interval relative to one core. */
	static float CPUTimePctRelative;

};

typedef FHoloLensTime FPlatformTime;
