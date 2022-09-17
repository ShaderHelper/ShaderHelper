// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "UObject/NameTypes.h"
#include "Features/IModularFeature.h"

#ifndef UE_EXTERNAL_PROFILING_ENABLED
// temporarily turn off profiler on Switch because of a compiler issue (?) with the thread_local init bools (switching 1 to int helped, 1 did not)
#define UE_EXTERNAL_PROFILING_ENABLED (!UE_BUILD_SHIPPING && !PLATFORM_SWITCH)
#endif

#if UE_EXTERNAL_PROFILING_ENABLED

/**
 * FExternalProfiler
 *
 * Interface to various external profiler API functions, dynamically linked
 */
class CORE_API FExternalProfiler : public IModularFeature
{

public:

	/** Constructor */
	FExternalProfiler();

	/** Empty virtual destructor. */
	virtual ~FExternalProfiler()
	{
	}

	/** Pauses profiling. */
	void PauseProfiler();

	/** Resumes profiling. */
	void ResumeProfiler();

	/**
	 * Profiler interface.
	 */

	/** Mark where the profiler should consider the frame boundary to be. */
	virtual void FrameSync() = 0;

	/** Initialize  profiler, register some delegates..*/
	virtual void Register() {}

	/** Pauses profiling. */
	virtual void ProfilerPauseFunction() = 0;

	/** Resumes profiling. */
	virtual void ProfilerResumeFunction() = 0;

	/** Gets the name of this profiler as a string.  This is used to allow the user to select this profiler in a system configuration file or on the command-line */
	virtual const TCHAR* GetProfilerName() const = 0;

	/** @return Returns the name to use for any profiler registered as a modular feature usable by this system */
	static FName GetFeatureName();

	/** Starts a scoped event specific to the profiler. */
	virtual void StartScopedEvent(const struct FColor& Color, const TCHAR* Text) {};

	/** Starts a scoped event specific to the profiler. Default implementation for backward compabitility. */
	virtual void StartScopedEvent(const struct FColor& Color, const ANSICHAR* Text) { StartScopedEvent(Color, ANSI_TO_TCHAR(Text)); };

	/** Ends a scoped event specific to the profiler. */
	virtual void EndScopedEvent() {};

	virtual void SetThreadName(const TCHAR* Name) {}

private:

	/** Number of timers currently running. Timers are always 'global inclusive'. */
	int32 TimerCount;

	/** Whether or not profiling is currently paused (as far as we know.) */
	bool bIsPaused;

	/** Friend class so we can access the private members directly. */
	friend class FScopedExternalProfilerBase;
};

class CORE_API FActiveExternalProfilerBase
{
public:	

	static FExternalProfiler* GetActiveProfiler() { return ActiveProfiler;	};

	static FExternalProfiler* InitActiveProfiler();
private:
	/** Static: True if we've tried to initialize a profiler already */
	static bool bDidInitialize;

	/** Static: Active profiler instance that we're using */
	static FExternalProfiler* ActiveProfiler;
};

/**
 * Base class for FScoped*Timer and FScoped*Excluder
 */
class FScopedExternalProfilerBase : public FActiveExternalProfilerBase
{
protected:
	/**
	 * Pauses or resumes profiler and keeps track of the prior state so it can be restored later.
	 *
	 * @param	bWantPause	true if this timer should 'include' code, or false to 'exclude' code
	 *
	 **/
	CORE_API void StartScopedTimer( const bool bWantPause );

	/** Stops the scoped timer and restores profiler to its prior state */
	CORE_API void StopScopedTimer();

private:
	/** Stores the previous 'Paused' state of VTune before this scope started */
	bool bWasPaused;
};


/**
 * FExternalProfilerIncluder
 *
 * Use this to include a body of code in profiler's captured session using 'Resume' and 'Pause' cues.  It
 * can safely be embedded within another 'timer' or 'excluder' scope.
 */
class FExternalProfilerIncluder : public FScopedExternalProfilerBase
{
public:
	/** Constructor */
	FExternalProfilerIncluder()
	{
		// 'Timer' scopes will always 'resume' VTune
		const bool bWantPause = false;
		StartScopedTimer( bWantPause );
	}

	/** Destructor */
	~FExternalProfilerIncluder()
	{
		StopScopedTimer();
	}
};


/**
 * FExternalProfilerExcluder
 *
 * Use this to EXCLUDE a body of code from profiler's captured session.  It can safely be embedded
 * within another 'timer' or 'excluder' scope.
 */
class FExternalProfilerExcluder : public FScopedExternalProfilerBase
{
public:
	/** Constructor */
	FExternalProfilerExcluder()
	{
		// 'Excluder' scopes will always 'pause' VTune
		const bool bWantPause = true;
		StartScopedTimer( bWantPause );
	}

	/** Destructor */
	~FExternalProfilerExcluder()
	{
		StopScopedTimer();
	}

};

class FExternalProfilerTrace
{
public:
	/** Starts a scoped event specific to the profiler. */
	FORCEINLINE static void StartScopedEvent(const struct FColor& Color, const TCHAR* Text)
	{
		FExternalProfiler* Profiler = FActiveExternalProfilerBase::GetActiveProfiler();
		if (Profiler)
		{
			Profiler->StartScopedEvent(Color, Text);
		}
	}

	/** Starts a scoped event specific to the profiler. */
	FORCEINLINE static void StartScopedEvent(const struct FColor& Color, const ANSICHAR* Text)
	{
		FExternalProfiler* Profiler = FActiveExternalProfilerBase::GetActiveProfiler();
		if (Profiler)
		{
			Profiler->StartScopedEvent(Color, Text);
		}
	}

	/** Ends a scoped event specific to the profiler. */
	FORCEINLINE static void EndScopedEvent()
	{
		FExternalProfiler* Profiler = FActiveExternalProfilerBase::GetActiveProfiler();
		if (Profiler)
		{
			Profiler->EndScopedEvent();
		}
	}
};

#define SCOPE_PROFILER_INCLUDER(X) FExternalProfilerIncluder ExternalProfilerIncluder_##X;
#define SCOPE_PROFILER_EXCLUDER(X) FExternalProfilerExcluder ExternalProfilerExcluder_##X;

#else	// UE_EXTERNAL_PROFILING_ENABLED

#define SCOPE_PROFILER_INCLUDER(X)
#define SCOPE_PROFILER_EXCLUDER(X)

#endif	// !UE_EXTERNAL_PROFILING_ENABLED