// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	HoloLensProperties.h - Basic static properties of a platform
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformProperties.h"
#include "HAL/Platform.h"

struct FHoloLensPlatformProperties : public FGenericPlatformProperties
{
	static FORCEINLINE const char* PlatformName()
	{
		return "HoloLens";
	}
	static FORCEINLINE const char* IniPlatformName()
	{
		return "HoloLens";
	}
	static FORCEINLINE const TCHAR* GetRuntimeSettingsClassName()
	{
		return TEXT("/Script/HoloLensRuntimeSettings.HoloLensRuntimeSettings");
	}
	static FORCEINLINE bool HasEditorOnlyData()
	{
		return false;
	}              
	static FORCEINLINE bool SupportsWindowedMode()
	{
		return true;
	}
	static FORCEINLINE bool RequiresCookedData()
	{
		return true;
	}
	static FORCEINLINE bool SupportsGrayscaleSRGB()
	{
		return false; // Requires expand from G8 to RGBA
	}

	static FORCEINLINE bool HasFixedResolution()
	{
		return false;
	}
	
	static FORCEINLINE bool SupportsQuit()
	{
		return true;
	}

	static FORCEINLINE bool SupportsLowQualityLightmaps()
	{
		// HoloLens 2 is mobile renderer, thus only supports low quality light maps
		return true;
	}

	static FORCEINLINE bool SupportsHighQualityLightmaps()
	{
		// HoloLens 2 is mobile renderer, thus only supports low quality light maps
		return false; 
	}

	static FORCEINLINE bool SupportsVirtualTextureStreaming()
	{
		return true;
	}
};

#ifdef PROPERTY_HEADER_SHOULD_DEFINE_TYPE
typedef FHoloLensPlatformProperties FPlatformProperties;
#endif
