// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "Containers/Map.h"

struct CORE_API FAndroidCrashContext : public FGenericCrashContext
{
	/** Signal number */
	int32 Signal;
	
	/** Additional signal info */
	siginfo* Info;
	
	/** Thread context */
	void* Context;

	FAndroidCrashContext(ECrashContextType InType, const TCHAR* InErrorMessage);

	~FAndroidCrashContext()
	{
	}

	/**
	 * Inits the crash context from data provided by a signal handler.
	 *
	 * @param InSignal number (SIGSEGV, etc)
	 * @param InInfo additional info (e.g. address we tried to read, etc)
	 * @param InContext thread context
	 */
	void InitFromSignal(int32 InSignal, siginfo* InInfo, void* InContext)
	{
		Signal = InSignal;
		Info = InInfo;
		Context = InContext;
	}

	virtual void GetPortableCallStack(const uint64* StackFrames, int32 NumStackFrames, TArray<FCrashStackFrame>& OutCallStack) const override;
	virtual void AddPlatformSpecificProperties() const override;

	void CaptureCrashInfo();
	void StoreCrashInfo(bool bWriteLog) const;

	static const int32 CrashReportMaxPathSize = 512;

	static void Initialize();
	// Returns the report directory used for this crash context.
	FString GetCurrentReportDirectoryPath() const {	return FString(ReportDirectory); }

	// Returns the main crash directory for this session. This will not be correct for non-fatal reports.
	static const FString GetGlobalCrashDirectoryPath();
	// Fills DirectoryNameOUT with the global crash directory for a fatal crash this session. This will not be correct for non-fatal reports.
	static void GetGlobalCrashDirectoryPath(char(&DirectoryNameOUT)[CrashReportMaxPathSize]);

	void AddAndroidCrashProperty(const FString& Key, const FString& Value);

	void SetOverrideCallstack(const FString& OverrideCallstackIN);

	// generate an absolute path to a crash report folder.
	static void GenerateReportDirectoryName(char(&DirectoryNameOUT)[CrashReportMaxPathSize]);

	void DumpAllThreadCallstacks() const;

	/** Async-safe ItoA */
	static const ANSICHAR* ItoANSI(uint64 Val, uint64 Base, uint32 Len = 0);

	// temporary accessor to allow overriding of the portable callstack.
	TArray<FCrashStackFrame>& GetPortableCallstack_TEMP() { return CallStack; }

protected:

	/** Allow platform implementations to provide a callstack property. Primarily used when non-native code triggers a crash. */
	virtual const TCHAR* GetCallstackProperty() const;

private:
	FString OverrideCallstack;

	TMap<FString, FString> AdditionalProperties;

	// The path used by this instance to store the report.
	char ReportDirectory[CrashReportMaxPathSize];
};

struct CORE_API FAndroidMemoryWarningContext : public FGenericMemoryWarningContext
{
	FAndroidMemoryWarningContext() : LastTrimMemoryState(FAndroidPlatformMemory::ETrimValues::Unknown) {}

	// value last recorded from java side's OnTrimMemory.
	FAndroidPlatformMemory::ETrimValues LastTrimMemoryState;
};

typedef FAndroidCrashContext FPlatformCrashContext;