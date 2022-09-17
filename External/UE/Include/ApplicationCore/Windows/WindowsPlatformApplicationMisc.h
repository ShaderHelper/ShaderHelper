// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct APPLICATIONCORE_API FWindowsPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static void PreInit();
	static void LoadStartupModules();
	static class FOutputDeviceConsole* CreateConsoleOutputDevice();
	static class FOutputDeviceError* GetErrorOutputDevice();
	static class FFeedbackContext* GetFeedbackContext();
	static class GenericApplication* CreateApplication();
	static void RequestMinimize();
	static bool IsThisApplicationForeground();
	static int32 GetAppIcon();
	static void PumpMessages(bool bFromMainLoop);
	static void PreventScreenSaver();
	static struct FLinearColor GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma = 1.0f);
	static void SetHighDPIMode();
	static bool GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle);
	static float GetDPIScaleFactorAtPoint(float X, float Y);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);

	/** Windows platform only */
	/** Function should retrieve the DPI value for the provided monitor information structure */
	static int32 GetMonitorDPI(const FMonitorInfo& MonitorInfo);
	static bool ProbablyHasIntegratedGPU();
	/** End Windows platform only */
};

#if WINDOWS_USE_FEATURE_APPLICATIONMISC_CLASS
typedef FWindowsPlatformApplicationMisc FPlatformApplicationMisc;
#endif
