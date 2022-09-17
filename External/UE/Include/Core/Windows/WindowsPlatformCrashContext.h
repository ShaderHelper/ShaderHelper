// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"

#if !defined(WINDOWS_CRASHCONTEXT_WITH_CUSTOM_HANDLERS)
	#define WINDOWS_CRASHCONTEXT_WITH_CUSTOM_HANDLERS 0
#endif


struct CORE_API FWindowsPlatformCrashContext : public FGenericCrashContext
{
	static const TCHAR* const UEGPUAftermathMinidumpName;
	
	FWindowsPlatformCrashContext(ECrashContextType InType, const TCHAR* InErrorMessage)
		: FGenericCrashContext(InType, InErrorMessage)
	{
	}

	virtual void SetPortableCallStack(const uint64* StackFrames, int32 NumStackFrames);

	virtual void AddPlatformSpecificProperties() const override;

	virtual void AddPortableThreadCallStack(uint32 ThreadId, const TCHAR* ThreadName, const uint64* StackFrames, int32 NumStackFrames) override;

	virtual void CopyPlatformSpecificFiles(const TCHAR* OutputDirectory, void* Context) override;

	void CaptureAllThreadContexts();

protected:
	virtual bool GetPlatformAllThreadContextsString(FString& OutStr) const override;

private:
	// Helpers
	typedef TArray<void*, TInlineAllocator<128>> FModuleHandleArray;
	
	static void GetProcModuleHandles(const FProcHandle& Process, FModuleHandleArray& OutHandles);

	static void ConvertProgramCountersToStackFrames(
		const FProcHandle& Process,
		const FModuleHandleArray& SortedModuleHandles,
		const uint64* ProgramCounters,
		int32 NumPCs,
		TArray<FCrashStackFrame>& OutStackFrames);

	static void AddThreadContextString(
		uint32 CrashedThreadId,
		uint32 ThreadId,
		const FString& ThreadName,
		const TArray<FCrashStackFrame>& StackFrames,
		FString& OutStr);	

};

typedef FWindowsPlatformCrashContext FPlatformCrashContext;
