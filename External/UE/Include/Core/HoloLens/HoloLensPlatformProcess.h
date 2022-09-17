// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	HoloLensProcess.h: HoloLens platform Process functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformProcess.h"

/** Dummy process handle for platforms that use generic implementation. */
struct FProcHandle : public TProcHandle<void*, nullptr>
{
public:
	/** Default constructor. */
	FORCEINLINE FProcHandle()
		: TProcHandle()
	{}

	/** Initialization constructor. */
	FORCEINLINE explicit FProcHandle( HandleType Other )
		: TProcHandle( Other )
	{}
};

/**
 * HoloLens implementation of the Process OS functions
 */
struct CORE_API FHoloLensProcess : public FGenericPlatformProcess
{
	static const TCHAR* BaseDir();
	static void Sleep( float Seconds );
	static void SleepNoStats(float Seconds); 
	[[noreturn]] static void SleepInfinite();
	static void YieldThread();
	UE_DEPRECATED(5.0, "Please use GetSynchEventFromPool to create a new event, and ReturnSynchEventToPool to release the event.")
	static class FEvent* CreateSynchEvent(bool bIsManualReset = false);
	static class FRunnableThread* CreateRunnableThread();
	static const TCHAR* ExecutableName(bool bRemoveExtension = true);
	static void* GetDllHandle(const TCHAR* Filename);
	static void FreeDllHandle(void* DllHandle);
	static void* GetDllExport(void* DllHandle, const TCHAR* ProcName);
	static void PushDllDirectory(const TCHAR* Directory);
	static void PopDllDirectory(const TCHAR* Directory);
	static void SetCurrentWorkingDirectoryToBaseDir();
	static FString GetCurrentWorkingDirectory();
	/** Content saved to the game or engine directories should be rerouted to user directories instead **/
	static bool ShouldSaveToUserDir();

	static void SetThreadAffinityMask( uint64 AffinityMask );
	static const TCHAR* UserDir();
	static const TCHAR* UserSettingsDir();
	static const TCHAR* UserTempDir();
	static const TCHAR* ApplicationSettingsDir();

	static const TCHAR* GetLocalAppDataLowLevelPath();
	static const TCHAR* GetTempAppDataLowLevelPath();
	static const TCHAR* GetLocalAppDataRedirectPath();
	static const TCHAR* GetTempAppDataRedirectPath();
	
	static bool CanLaunchURL(const TCHAR* URL);
	static void LaunchURL(const TCHAR* URL, const TCHAR* Parms, FString* Error);

private:
	/**
	* Since Windows can only have one directory at a time,
	* this stack is used to reset the previous DllDirectory.
	*/
	static TArray<FString> DllDirectoryStack;
};

typedef FHoloLensProcess FPlatformProcess;

