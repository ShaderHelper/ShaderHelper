// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"

/** Passed in through sigqueue for gathering of a callstack from a signal */
struct ThreadStackUserData
{
	// If we want a backtrace or a callstack
	// Backtrace is just a list of program counters and callstack is a symbolicated backtrace
	bool bCaptureCallStack;
	union 
	{
		ANSICHAR* CallStack;
		uint64* BackTrace;
	};

	int32 BackTraceCount;
	SIZE_T CallStackSize;
	TAtomic<bool> bDone;
};

struct CORE_API FUnixCrashContext : public FGenericCrashContext
{
	/** Signal number */
	int32 Signal;
	
	/** Additional signal info */
	siginfo_t* Info;
	
	/** Thread context */
	ucontext_t*	Context;

	/** Whether backtrace was already captured */
	bool bCapturedBacktrace;

	/** Symbols received via backtrace_symbols(), if any (note that we will need to clean it up) */
	char ** BacktraceSymbols;

	/** Memory reserved for "exception" (signal) info */
	TCHAR SignalDescription[256];

	/** Memory reserved for minidump-style callstack info */
	char MinidumpCallstackInfo[16384];

	/** Fake siginfo used when handling ensure()s */
	static __thread siginfo_t	FakeSiginfoForEnsures;

	/** The PC of the first function used when handling a crash. Used to figure out the number of frames to ignore */
	uint64* FirstCrashHandlerFrame = nullptr;

	/** The PC of where the error being reported occurred. Note that this could be different
	 * from FirstCrashHandlerFrame */
	void* ErrorFrame = nullptr;

	FUnixCrashContext(ECrashContextType InType, const TCHAR* InErrorMessage)
		:	FGenericCrashContext(InType, InErrorMessage)
		,	Signal(0)
		,	Info(nullptr)
		,	Context(nullptr)
		,	bCapturedBacktrace(false)
		,	BacktraceSymbols(nullptr)
	{
		SignalDescription[ 0 ] = 0;
		MinidumpCallstackInfo[ 0 ] = 0;
	}

	~FUnixCrashContext();

	/**
	 * Inits the crash context from data provided by a signal handler.
	 *
	 * @param InSignal number (SIGSEGV, etc)
	 * @param InInfo additional info (e.g. address we tried to read, etc)
	 * @param InContext thread context
	 */
	void InitFromSignal(int32 InSignal, siginfo_t* InInfo, void* InContext);

	/**
	 * Inits the crash context from ensure handler
	 *
	 * @param EnsureMessage Message about the ensure that failed
	 * @param CrashAddress address where "crash" happened
	 */
	void InitFromEnsureHandler(const TCHAR* EnsureMessage, const void* CrashAddress);

	/**
	 * Populates crash context stack trace and a few related fields
	 *
	 */
	void CaptureStackTrace(void* ErrorProgramCounter);

	/**
	 * Generates a new crash report containing information needed for the crash reporter and launches it; may not return.
	 *
	 * @param bReportingNonCrash if true, we are not reporting a crash (but e.g. ensure()), so don't re-raise the signal.
	 *
	 * @return If bReportingNonCrash is false, the function will not return
	 */
	void GenerateCrashInfoAndLaunchReporter(bool bReportingNonCrash = false) const;

	/**
	 * Sets whether this crash represents a non-crash event like an ensure
	 */
	void SetType(ECrashContextType InType) { Type = InType; }

	/**
	 * Sets the FirstCrashHandlerFrame only if it has not been set before
	 */
	void SetFirstCrashHandlerFrame(uint64* ProgramCounter);

	virtual void GetPortableCallStack(const uint64* StackFrames, int32 NumStackFrames, TArray<FCrashStackFrame>& OutCallStack) const override;

	void AddPlatformSpecificProperties() const;

protected:
	/**
	 * Dumps all the data from crash context to the "minidump" report.
	 *
	 * @param DiagnosticsPath Path to put the file to
	 */
	void GenerateReport(const FString & DiagnosticsPath) const;
};

typedef FUnixCrashContext FPlatformCrashContext;

namespace UnixCrashReporterTracker
{
	/**
	 * Initialize persistent data and tickers
	 */
	void PreInit();

	/**
	 * Only call this function from a forked child process. The child process cannot be responsible for a sibling process.
	 *
	 * This removes a valid Crash Reporter tracker from the calling process.
	 */
	void RemoveValidCrashReportTickerForChildProcess();
}
