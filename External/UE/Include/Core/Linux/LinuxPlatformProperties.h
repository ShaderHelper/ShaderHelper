// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	LinuxPlatformProperties.h - Basic static properties of a platform 
	These are shared between:
		the runtime platform - via FPlatformProperties
		the target platforms - via ITargetPlatform
==================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformProperties.h"


/**
 * Implements Linux platform properties.
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY, bool IS_ARM64>
struct FLinuxPlatformProperties
	: public FGenericPlatformProperties
{
	static FORCEINLINE bool HasEditorOnlyData( )
	{
		return HAS_EDITOR_DATA;
	}

	static FORCEINLINE const char* IniPlatformName( )
	{
		return IS_ARM64 ? "LinuxArm64" : "Linux";
	}

	static FORCEINLINE const TCHAR* GetRuntimeSettingsClassName()
	{
		return TEXT("/Script/LinuxTargetPlatform.LinuxTargetSettings");
	}

	static FORCEINLINE bool IsGameOnly( )
	{
		return UE_GAME;
	}

	static FORCEINLINE bool IsServerOnly( )
	{
		return IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool IsClientOnly()
	{
		return IS_CLIENT_ONLY;
	}

	static FORCEINLINE bool IsArm64()
	{
		return IS_ARM64;
	}

	static FORCEINLINE const char* PlatformName( )
	{
		if (IS_DEDICATED_SERVER)
		{
			return IS_ARM64 ? "LinuxArm64Server" : "LinuxServer";
		}

		if (HAS_EDITOR_DATA)
		{
			return "LinuxEditor";
		}

		if (IS_CLIENT_ONLY)
		{
			return IS_ARM64 ? "LinuxArm64Client" : "LinuxClient";
		}

		return IS_ARM64 ? "LinuxArm64" : "Linux";
	}

	static FORCEINLINE bool RequiresCookedData( )
	{
		return !HAS_EDITOR_DATA;
	}

	static FORCEINLINE bool HasSecurePackageFormat()
	{
		return IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool RequiresUserCredentials()
	{
		return true;
	}

	static FORCEINLINE bool SupportsAutoSDK()
	{
// linux cross-compiling / cross-building from windows supports AutoSDK.  But hosted linux doesn't yet.
#if PLATFORM_WINDOWS
		return true;
#else
		return false;
#endif
	}

	static FORCEINLINE bool SupportsMultipleGameInstances( )
	{
		return true;
	}

	static FORCEINLINE bool HasFixedResolution()
	{
		return false;
	}

	static FORCEINLINE bool SupportsWindowedMode()
	{
		return !IS_DEDICATED_SERVER;
	}

	static FORCEINLINE bool AllowsFramerateSmoothing()
	{
		return true;
	}

	static FORCEINLINE bool SupportsQuit()
	{
		return true;
	}

	static FORCEINLINE float GetVariantPriority()
	{
		if (IS_DEDICATED_SERVER)
		{
			return 0.0f;
		}

		if (HAS_EDITOR_DATA)
		{
			return 0.0f;
		}

		if (IS_CLIENT_ONLY)
		{
			return 0.0f;
		}

		return 1.0f;
	}

	static FORCEINLINE bool AllowsCallStackDumpDuringAssert()
	{
		return true;
	}

	static FORCEINLINE bool SupportsAudioStreaming()
	{
		return !IsServerOnly();
	}

	static FORCEINLINE bool SupportsVirtualTextureStreaming()
	{
		return true;
	}

};

#ifdef PROPERTY_HEADER_SHOULD_DEFINE_TYPE
typedef FLinuxPlatformProperties<WITH_EDITORONLY_DATA, UE_SERVER, !WITH_SERVER_CODE && !WITH_EDITOR, !!PLATFORM_CPU_ARM_FAMILY> FPlatformProperties;
#endif
