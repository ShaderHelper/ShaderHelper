// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	UnixPlatformTime.h: Unix platform Time functions
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformTime.h"

/**
 * Unix implementation of the Time OS functions
 */
struct CORE_API FUnixTime : public FGenericPlatformTime
{
	static double InitTiming();

	static FORCEINLINE double Seconds()
	{
		if (UNLIKELY(ClockSource < 0))
		{
			ClockSource = CalibrateAndSelectClock();
		}

		struct timespec ts;
		clock_gettime(ClockSource, &ts);
		return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
	}

	static FORCEINLINE uint32 Cycles()
	{
		if (UNLIKELY(ClockSource < 0))
		{
			ClockSource = CalibrateAndSelectClock();
		}

		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return static_cast<uint32>(static_cast<uint64>(ts.tv_sec) * (uint64)1e6 + static_cast<uint64>(ts.tv_nsec) / 1000ULL);
	}

	static FORCEINLINE uint64 Cycles64()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return static_cast<uint64>(static_cast<uint64>(ts.tv_sec) * (uint64)1e7 + static_cast<uint64>(ts.tv_nsec) / 100ULL);
	}

	static bool UpdateCPUTime(float DeltaSeconds);
	static bool UpdateThreadCPUTime(float = 0.0);
	static void AutoUpdateGameThreadCPUTime(double UpdateInterval);

	static FCPUTime GetCPUTime();
	static FCPUTime GetThreadCPUTime();
	static double GetLastIntervalThreadCPUTimeInSeconds();

	/**
	 * Calibration log to be printed at later time
	 */
	static void PrintCalibrationLog();

private:

	/** Clock source to use */
	static int ClockSource;

	/** Log information about calibrating the clock. */
	static char CalibrationLog[4096];

	/**
	 * Benchmarks clock_gettime(), possibly switches to something else is too slow.
	 * Unix-specific.
	 */
	static int CalibrateAndSelectClock();

	/**
	 * Returns number of time we can call the clock per second.
	 */
	static uint64 CallsPerSecondBenchmark(clockid_t BenchClockId, const char * BenchClockIdName);
};

typedef FUnixTime FPlatformTime;