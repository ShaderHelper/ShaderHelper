// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	HoloLensMisc.h: HoloLens platform misc functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMisc.h"
#include "HAL/Platform.h"
#include "Misc/Build.h"
#include "HoloLens/HoloLensSystemIncludes.h"


#define UE_DEBUG_BREAK_IMPL() PLATFORM_BREAK()

class FString;

/**
 * HoloLens implementation of the misc OS functions
 */
struct CORE_API FHoloLensMisc : public FGenericPlatformMisc
{
	static void PlatformPreInit();
	static void PlatformInit();
	static void PlatformPostInit(bool ShowSplashScreen = false);
	static FString GetEnvironmentVariable(const TCHAR* VariableName);
	static const TCHAR* GetPlatformFeaturesModuleName();

#if !UE_BUILD_SHIPPING
	static bool IsDebuggerPresent();
	FORCEINLINE static void DebugBreak()
	{
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
	}
#endif

	/** Break into debugger. Returning false allows this function to be used in conditionals. */
	FORCEINLINE static bool DebugBreakReturningFalse()
	{
#if !UE_BUILD_SHIPPING
		DebugBreak();
#endif
		return false;
	}

    /** Prompts for remote debugging if debugger is not attached. Regardless of result, breaks into debugger afterwards. Returns false for use in conditionals. */
    static FORCEINLINE bool DebugBreakAndPromptForRemoteReturningFalse(bool bIsEnsure = false)
    {
#if !UE_BUILD_SHIPPING
        if (!IsDebuggerPresent())
        {
            PromptForRemoteDebugging(bIsEnsure);
        }

        DebugBreak();
#endif
        return false;
    }
    
    static void PumpMessages(bool bFromMainLoop);
	static void LowLevelOutputDebugString(const TCHAR *Message);
	static void RequestExit(bool Force);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);
	static void CreateGuid(struct FGuid& Result);
	static int32 NumberOfCores();
	static EAppReturnType::Type MessageBoxExt(EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption);

	static bool CoInitialize();
	static void CoUninitialize();

	/** Get the application root directory. */
	static const TCHAR* RootDir();

	/**
	* Function to save the URI string from a protocol activation for use elsewhere in the game.
	*/
	static void SetProtocolActivationUri(const FString& NewUriString);

	/**
	* Function to get the URI string from the most recent protocol activation.
	*/
	static const FString& GetProtocolActivationUri();

	static bool SupportsLocalCaching()
	{
		return false;
	}

	static void GetValidTargetPlatforms(TArray<FString>& TargetPlatformNames);

	static bool VerifyWindowsVersion(uint32 MajorVersion, uint32 MinorVersion, uint32 BuildNumber = 0);

private:

	/** character buffer containing the last protocol activation URI */
	static FString ProtocolActivationUri;
};

typedef FHoloLensMisc FPlatformMisc;
