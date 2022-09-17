// Copyright Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	UnixPlatformMisc.h: Unix platform misc functions
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/Build.h"

#define UE_DEBUG_BREAK_IMPL() FUnixPlatformMisc::UngrabAllInput(); PLATFORM_BREAK()

class Error;
struct FGenericCrashContext;

/**
 * Unix implementation of the misc OS functions
 */
struct CORE_API FUnixPlatformMisc : public FGenericPlatformMisc
{
	static void PlatformPreInit();
	static void PlatformInit();
	static void PlatformTearDown();
	static void SetGracefulTerminationHandler();
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext& Context));
	static int32 GetMaxPathLength();

	UE_DEPRECATED(4.21, "void FPlatformMisc::GetEnvironmentVariable(Name, Result, Length) is deprecated. Use FString FPlatformMisc::GetEnvironmentVariable(Name) instead.")
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength);

	static FString GetEnvironmentVariable(const TCHAR* VariableName);
	static void SetEnvironmentVar(const TCHAR* VariableName, const TCHAR* Value);
	static TArray<uint8> GetMacAddress();
	static bool IsRunningOnBattery();

#if !UE_BUILD_SHIPPING
	static bool IsDebuggerPresent();
#endif // !UE_BUILD_SHIPPING

	static void LowLevelOutputDebugString(const TCHAR *Message);

	static void RequestExit(bool Force);
	static void RequestExitWithStatus(bool Force, uint8 ReturnCode);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);

	static void NormalizePath(FString& InPath);
	static void NormalizePath(FStringBuilderBase& InPath);

	static const TCHAR* GetPathVarDelimiter()
	{
		return TEXT(":");
	}

	static EAppReturnType::Type MessageBoxExt(EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption);

	FORCEINLINE static void MemoryBarrier()
	{
		__sync_synchronize();
	}

	FORCEINLINE static void PrefetchBlock(const void* InPtr, int32 NumBytes = 1)
	{
		extern size_t GCacheLineSize;

		const char* Ptr = static_cast<const char*>(InPtr);
		const size_t CacheLineSize = GCacheLineSize;
		for (size_t BytesPrefetched = 0; BytesPrefetched < NumBytes; BytesPrefetched += CacheLineSize)
		{
			__builtin_prefetch(Ptr);
			Ptr += CacheLineSize;
		}
	}

	FORCEINLINE static void Prefetch(void const* Ptr, int32 Offset = 0)
	{
		__builtin_prefetch(static_cast<char const*>(Ptr) + Offset);
	}

	static int32 NumberOfCores();
	static int32 NumberOfCoresIncludingHyperthreads();
	static FString GetOperatingSystemId();
	static bool GetDiskTotalAndFreeSpace(const FString& InPath, uint64& TotalNumberOfBytes, uint64& NumberOfFreeBytes);
	static bool GetPageFaultStats(FPageFaultStats& OutStats, EPageFaultFlags Flags=EPageFaultFlags::All);
	static bool GetBlockingIOStats(FProcessIOStats& OutStats, EInputOutputFlags Flags=EInputOutputFlags::All);
	static bool GetContextSwitchStats(FContextSwitchStats& OutStats, EContextSwitchFlags Flags=EContextSwitchFlags::All);

	/**
	 * Determines the shader format for the platform
	 *
	 * @return	Returns the shader format to be used by that platform
	 */
	static const TCHAR* GetNullRHIShaderFormat();

	static bool HasCPUIDInstruction();
	static FString GetCPUVendor();
	static FString GetCPUBrand();

	/**
	 * Uses cpuid instruction to get the vendor string
	 *
	 * @return	CPU info bitfield
	 *
	 *			Bits 0-3	Stepping ID
	 *			Bits 4-7	Model
	 *			Bits 8-11	Family
	 *			Bits 12-13	Processor type (Intel) / Reserved (AMD)
	 *			Bits 14-15	Reserved
	 *			Bits 16-19	Extended model
	 *			Bits 20-27	Extended family
	 *			Bits 28-31	Reserved
	 */
	static uint32 GetCPUInfo();

	static bool HasNonoptionalCPUFeatures();
	static bool NeedsNonoptionalCPUFeaturesCheck();

	/**
	 * Ungrabs input (useful before breaking into debugging)
	 */
	static void UngrabAllInput();

	/**
	 * Returns whether the program has been started remotely (e.g. over SSH)
	 */
	static bool HasBeenStartedRemotely();

	/**
	 * Determines if return code has been overriden and returns it.
	 *
	 * @param OverriddenReturnCodeToUsePtr pointer to an variable that will hold an overriden return code, if any. Can be null.
	 *
	 * @return true if the error code has been overriden, false if not
	 */
	static bool HasOverriddenReturnCode(uint8 * OverriddenReturnCodeToUsePtr);
	static void GetOSVersions(FString& out_OSVersionLabel, FString& out_OSSubVersionLabel);
	static FString GetOSVersion();
	static FString GetLoginId();

	static void CreateGuid(FGuid& Result);

	static IPlatformChunkInstall* GetPlatformChunkInstall();

	static bool SetStoredValues(const FString& InStoreId, const FString& InSectionName, const TMap<FString, FString>& InKeyValues);

	static int32 NumberOfWorkerThreadsToSpawn();

#if STATS || ENABLE_STATNAMEDEVENTS
	static void BeginNamedEventFrame();
	static void BeginNamedEvent(const struct FColor& Color, const TCHAR* Text);
	static void BeginNamedEvent(const struct FColor& Color, const ANSICHAR* Text);
	static void EndNamedEvent();
	static void CustomNamedStat(const TCHAR* Text, float Value, const TCHAR* Graph, const TCHAR* Unit);
	static void CustomNamedStat(const ANSICHAR* Text, float Value, const ANSICHAR* Graph, const ANSICHAR* Unit);
#endif
};
