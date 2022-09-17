// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformStackWalk.h"

/**
 * Windows implementation of the stack walking.
 **/
struct CORE_API FWindowsPlatformStackWalk
	: public FGenericPlatformStackWalk
{
	static bool InitStackWalking();
	static bool InitStackWalkingForProcess(const FProcHandle& Process);
	
	static TArray<FProgramCounterSymbolInfo> GetStack(int32 IgnoreCount, int32 MaxDepth = 100, void* Context = nullptr);

	static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo& out_SymbolInfo );
	static void ProgramCounterToSymbolInfoEx( uint64 ProgramCounter, FProgramCounterSymbolInfoEx& out_SymbolInfo );
	static uint32 CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context = nullptr );
	static uint32 CaptureThreadStackBackTrace( uint64 ThreadId, uint64* BackTrace, uint32 MaxDepth, void* Context = nullptr);
	static void StackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, int32 IgnoreCount, void* Context = nullptr );
	static void StackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, void* ProgramCounter, void* Context = nullptr );
	static void ThreadStackWalkAndDump(ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, int32 IgnoreCount, uint32 ThreadId);

	static int32 GetProcessModuleCount();
	static int32 GetProcessModuleSignatures(FStackWalkModuleInfo *ModuleSignatures, const int32 ModuleSignaturesSize);

	static void RegisterOnModulesChanged();

	/**
	 * Upload localy built symbols to network symbol storage.
	 *
	 * Use case:
	 *   Game designers use game from source (without prebuild game .dll-files).
	 *   In this case all game .dll-files are compiled locally.
	 *   For post-mortem debug programmers need .dll and .pdb files from designers.
	 */
	static bool UploadLocalSymbols();

	/**
	 * Get downstream storage with downloaded from remote symbol storage files.
	 */
	static FString GetDownstreamStorage();

	static void* MakeThreadContextWrapper(void* Context, void* ThreadHandle);
	static void ReleaseThreadContextWrapper(void* ThreadContext);

	/**
	 * Returns the source file pathname, line and column where the specified function is defined.
	 *
	 * The implementation extracts the information from the debug engine and the debug symbols and
	 * takes care of loading the debug symbols if the debug engine was configured to load symbols
	 * on demand. This function can be expensive if the debug symbols needs to be loaded.
	 *
	 * @param FunctionSymbolName The function name to lookup.
	 * @param FunctionModuleName The module name containing the function to lookup.
	 * @param OutPathname The source file pathname.
	 * @param OutLineNumber The line at which the function is defined in the source file.
	 * @param OutColumnNumber The offset on the line at which the function is defined in the source file.
	 * @return True if the the function location is found, false otherwise.
	 */
	static bool GetFunctionDefinitionLocation(const FString& FunctionSymbolName, const FString& FunctionModuleName, FString& OutPathname, uint32& OutLineNumber, uint32& OutColumnNumber);

private:
	static bool InitStackWalkingInternal(void* Process, bool bForceReinitOnProcessMismatch);
};


typedef FWindowsPlatformStackWalk FPlatformStackWalk;
