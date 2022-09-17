// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	IOSPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "GenericPlatform/GenericPlatformProperties.h"


/**
 * Implements iOS platform properties.
 */
struct FIOSPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return false;
	}

	static FORCEINLINE const char* PlatformName( )
	{
		return "IOS";
	}

	static FORCEINLINE const char* IniPlatformName( )
	{
		return "IOS";
	}

	static FORCEINLINE const TCHAR* GetRuntimeSettingsClassName()
	{
		return TEXT("/Script/IOSRuntimeSettings.IOSRuntimeSettings");
	}

	static FORCEINLINE bool IsGameOnly()
	{
		return true;
	}
	
	static FORCEINLINE bool RequiresCookedData( )
	{
		return true;
	}
    
	static FORCEINLINE bool SupportsBuildTarget( EBuildTargetType TargetType )
	{
		return (TargetType == EBuildTargetType::Game);
	}

	static FORCEINLINE bool SupportsLowQualityLightmaps()
	{
		return true;
	}

	static FORCEINLINE bool SupportsHighQualityLightmaps()
	{
		return true;
	}

	static FORCEINLINE bool SupportsTextureStreaming()
	{
		return true;
	}

	static FORCEINLINE bool SupportsMemoryMappedFiles()
	{
		return true;
	}
	static FORCEINLINE bool SupportsMemoryMappedAudio()
	{
		return true;
	}
	static FORCEINLINE bool SupportsMemoryMappedAnimation()
	{
		return false;
	}
	static FORCEINLINE int64 GetMemoryMappingAlignment()
	{
		return 16384;
	}

	static FORCEINLINE bool HasFixedResolution()
	{
		return true;
	}

	static FORCEINLINE bool AllowsFramerateSmoothing()
	{
		return true;
	}

	static FORCEINLINE bool SupportsAudioStreaming()
	{
		return true;
	}
	
	static FORCEINLINE bool SupportsMeshLODStreaming()
	{
		return true;
	}

	static FORCEINLINE bool SupportsVirtualTextureStreaming()
	{
		return true;
	}
};

struct FTVOSPlatformProperties : public FIOSPlatformProperties
{
	// @todo breaking change here!
	static FORCEINLINE const char* PlatformName()
	{
		return "TVOS";
	}

	static FORCEINLINE const char* IniPlatformName()
	{
		return "TVOS";
	}
};

#ifdef PROPERTY_HEADER_SHOULD_DEFINE_TYPE


typedef FIOSPlatformProperties FPlatformProperties;

#endif
