// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformMisc.h: Apple platform misc functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMisc.h"
#if PLATFORM_MAC
#include "Mac/MacSystemIncludes.h"
#else
#include "IOS/IOSSystemIncludes.h"
#endif

#ifndef APPLE_PROFILING_ENABLED
#define APPLE_PROFILING_ENABLED (UE_BUILD_DEBUG | UE_BUILD_DEVELOPMENT)
#endif

#ifndef WITH_SIMULATOR
#define WITH_SIMULATOR 0
#endif

#define UE_DEBUG_BREAK_IMPL() PLATFORM_BREAK()

extern CORE_API bool GIgnoreDebugger;

#ifdef __OBJC__

class FScopeAutoreleasePool
{
public:

	FScopeAutoreleasePool()
	{
		Pool = [[NSAutoreleasePool alloc] init];
	}

	~FScopeAutoreleasePool()
	{
		[Pool release];
	}

private:

	NSAutoreleasePool*	Pool;
};

#define SCOPED_AUTORELEASE_POOL const FScopeAutoreleasePool PREPROCESSOR_JOIN(Pool,__LINE__);

#endif // __OBJC__

/**
* Apple implementation of the misc OS functions
**/
struct CORE_API FApplePlatformMisc : public FGenericPlatformMisc
{
	static void PlatformInit();

	UE_DEPRECATED(4.21, "void FPlatformMisc::GetEnvironmentVariable(Name, Result, Length) is deprecated. Use FString FPlatformMisc::GetEnvironmentVariable(Name) instead.")
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength);

	static FString GetEnvironmentVariable(const TCHAR* VariableName);

#if !UE_BUILD_SHIPPING
	static bool IsDebuggerPresent()
	{
		// Based on http://developer.apple.com/library/mac/#qa/qa1361/_index.html

		if (GIgnoreDebugger)
		{
			return false;
		}

		struct kinfo_proc Info;
		int32 Mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
		SIZE_T Size = sizeof(Info);

		sysctl( Mib, sizeof( Mib ) / sizeof( *Mib ), &Info, &Size, NULL, 0 );

		return ( Info.kp_proc.p_flag & P_TRACED ) != 0;
	}
#endif

	FORCEINLINE static void MemoryBarrier()
	{
		__sync_synchronize();
	}

	static void LocalPrint(const TCHAR* Message);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);
	static int32 NumberOfCores();
	
	/** @return Whether filehandles can be opened on one thread and read/written on another thread */
	static bool SupportsMultithreadedFileHandles()
	{
		// ApplePlatformFile currently uses thread-local lists that can close filehandles arbitrarily and reopen them at need, so filehandles are not transferrable between threads
		return false;
	}

	static void CreateGuid(struct FGuid& Result);
	static TArray<uint8> GetSystemFontBytes();
	static FString GetDefaultLocale();
	static FString GetDefaultLanguage();
	static FString GetLocalCurrencyCode();
	static FString GetLocalCurrencySymbol();

	static bool IsOSAtLeastVersion(const uint32 MacOSVersion[3], const uint32 IOSVersion[3], const uint32 TVOSVersion[3]);

#if STATS || ENABLE_STATNAMEDEVENTS || APPLE_PROFILING_ENABLED
	static void BeginNamedEventFrame();
	static void BeginNamedEvent(const struct FColor& Color, const TCHAR* Text);
	static void BeginNamedEvent(const struct FColor& Color, const ANSICHAR* Text);
	static void EndNamedEvent();
	static void CustomNamedStat(const TCHAR* Text, float Value, const TCHAR* Graph, const TCHAR* Unit);
	static void CustomNamedStat(const ANSICHAR* Text, float Value, const ANSICHAR* Graph, const ANSICHAR* Unit);
#endif

	//////// Platform specific
	static void* CreateAutoreleasePool();
	static void ReleaseAutoreleasePool(void *Pool);
};
