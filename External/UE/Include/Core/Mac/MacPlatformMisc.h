// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MacPlatformMisc.h: Mac platform misc functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Mac/MacSystemIncludes.h"
#include "Apple/ApplePlatformMisc.h"

/**
* Mac implementation of the misc OS functions
**/
struct CORE_API FMacPlatformMisc : public FApplePlatformMisc
{
	static void PlatformPreInit();
	static void PlatformInit();
	static void PlatformTearDown();
	static void SetEnvironmentVar(const TCHAR* VariableName, const TCHAR* Value);

	FORCEINLINE static int32 GetMaxPathLength()
	{
		return MAC_MAX_PATH;
	}

	static const TCHAR* GetPathVarDelimiter()
	{
		return TEXT(":");
	}

	static TArray<uint8> GetMacAddress();

	static void RequestExit(bool Force);
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );
	static bool CommandLineCommands();
	static int32 NumberOfCores();
	static int32 NumberOfCoresIncludingHyperthreads();
	static void NormalizePath(FString& InPath);
	static void NormalizePath(FStringBuilderBase& InPath);
	static FString GetPrimaryGPUBrand();
	static struct FGPUDriverInfo GetGPUDriverInfo(const FString& DeviceDescription);
	static void GetOSVersions( FString& out_OSVersionLabel, FString& out_OSSubVersionLabel );
	static FString GetOSVersion();
	static bool HasPlatformFeature(const TCHAR* FeatureName);
	static bool GetDiskTotalAndFreeSpace(const FString& InPath, uint64& TotalNumberOfBytes, uint64& NumberOfFreeBytes);
	static bool HasSeparateChannelForDebugOutput();

	/** 
	 * Determines the shader format for the plarform
	 *
	 * @return	Returns the shader format to be used by that platform
	 */
	FORCEINLINE static const TCHAR* GetNullRHIShaderFormat() 
	{ 
		return TEXT("SF_METAL_SM5"); 
	}

	/**
	 * Uses cpuid instruction to get the vendor string
	 *
	 * @return	CPU vendor name
	 */
	static FString GetCPUVendor();

	/**
	 * Uses cpuid instruction to get the CPU brand string
	 *
	 * @return    CPU brand string
	 */
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

    static bool HasNonoptionalCPUFeatures() { return true; }
    static bool NeedsNonoptionalCPUFeaturesCheck() { return PLATFORM_ENABLE_POPCNT_INTRINSIC; }

	static void SetGracefulTerminationHandler();
	
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext& Context));

	/** @return Get the name of the platform specific file manager (Finder) */
	static FText GetFileManagerName();

	/**
	 * Returns whether the platform is running on battery power or not.
	 */
	static bool IsRunningOnBattery();

	static IPlatformChunkInstall* GetPlatformChunkInstall();

	/**
	 * Returns whether the Mac OS X version is 10.9.x or not.
	 */
	static bool IsRunningOnMavericks();

	/**
	 * Returns if current < target returns 1, if current > target returns 1, else current == target and returns 0.
	 */
	static int32 MacOSXVersionCompare(uint8 Major, uint8 Minor, uint8 Revision);

	/**
	 * Gets a globally unique ID the represents a particular operating system install.
	 */
	static FString GetOperatingSystemId();

	static FString GetXcodePath();

	static bool IsSupportedXcodeVersionInstalled();

#if WITH_EDITOR
	static bool IsRunningOnRecommendedMinSpecHardware();
#endif // WITH_EDITOR

	static void MergeDefaultArgumentsIntoCommandLine(FString& CommandLine, FString DefaultArguments);
	
	/** Common descriptor of each GPU in the OS that provides stock details about the GPU that are innaccessible from the higher-level rendering APIs and provides a direct link to the GPU in the IORegistry. */
	template<typename T>
	class FGPUDescriptorCommon
	{
		public:
			virtual ~FGPUDescriptorCommon();

			FGPUDescriptorCommon& operator=(FGPUDescriptorCommon const& Other);

			TMap<FString, float> GetPerformanceStatistics() const;

		protected:
			FGPUDescriptorCommon() = default;
			FGPUDescriptorCommon(FGPUDescriptorCommon const& Other) = delete;
			FGPUDescriptorCommon(FGPUDescriptorCommon&& Other) = delete;

			void CopyFrom(FGPUDescriptorCommon const& Other);

		public:
			NSString*	GPUName			= nil;
			NSString*	GPUMetalBundle	= nil;
			NSString*	GPUOpenGLBundle	= nil;
			NSString*	GPUBundleID		= nil;
			uint32		GPUVendorId		= 0;
			uint32		GPUDeviceId		= 0;
			uint32		GPUMemoryMB		= 0;
			uint32		GPUIndex		= 0;

			bool GPUHeadless			= false;
	};

#if PLATFORM_MAC_X86
	/** Intel architecture descriptor of each GPU in the OS that provides stock details about the GPU that are innaccessible from the higher-level rendering APIs and provides a direct link to the GPU in the IORegistry. */
	typedef class FGPUDescriptorX86_64 : public FGPUDescriptorCommon<FGPUDescriptorX86_64>
	{
		friend class FGPUDescriptorCommon<FGPUDescriptorX86_64>;

		public:
			FGPUDescriptorX86_64() = default;
			FGPUDescriptorX86_64(FGPUDescriptorX86_64 const& Other);

			virtual ~FGPUDescriptorX86_64();

		protected:
			void CopyFromImpl(FGPUDescriptorCommon const& Other);
			TMap<FString, float> GetPerformanceStatisticsImpl() const;

		public:
			uint64 RegistryID;
			uint32 PCIDevice; // This is really an io_registry_entry_t which is a mach port name

	} FGPUDescriptor;

#elif PLATFORM_MAC_ARM64
	typedef class FGPUDescriptorARM64 : public FGPUDescriptorCommon<FGPUDescriptorARM64>
	{
		friend class FGPUDescriptorCommon<FGPUDescriptorARM64>;

		public:
			FGPUDescriptorARM64() = default;
			FGPUDescriptorARM64(FGPUDescriptorARM64 const& Other);

			virtual ~FGPUDescriptorARM64();

		protected:
			void CopyFromImpl(FGPUDescriptorCommon const& Other);
			TMap<FString, float> GetPerformanceStatisticsImpl() const;

		public:
			uint64 RegistryID = 0;

	} FGPUDescriptor;

#else
	#error "Undefined Mac platform"
#endif

	enum class EMacGPUNotification : uint8
	{
		Added,
		RemovalRequested,
		Removed
	};
	
	/** Handle GPU change notifications. */
	static void GPUChangeNotification(uint64_t DeviceRegistryID, EMacGPUNotification Notification);
	
	/** Returns the static list of GPUs in the current machine. */
	static TArray<FGPUDescriptor> const& GetGPUDescriptors();
	
	/** Returns the index of the GPU to use or -1 if we should just use the default. */
	static int32 GetExplicitRendererIndex();
    
    /** Update the driver monitor statistics for the given GPU - called once a frame by the Mac RHI's, no need to call otherwise - use GetPerformanceStatistics instead. */
    static void UpdateDriverMonitorStatistics(int32 DeviceIndex);

	static int GetDefaultStackSize();

	/** Updates variables in GMacAppInfo that cannot be initialized before PlatformPostInit() */
	static void PostInitMacAppInfoUpdate();

	static CGDisplayModeRef GetSupportedDisplayMode(CGDirectDisplayID DisplayID, uint32 Width, uint32 Height);

	FORCEINLINE static void ChooseHDRDeviceAndColorGamut(uint32 DeviceId, uint32 DisplayNitLevel, int32& OutputDevice, int32& ColorGamut)
	{
		// ScRGB, 1000 or 2000 nits, DCI-P3
		OutputDevice = DisplayNitLevel == 1000 ? 5 : 6;
		ColorGamut = 1;
	}
};

typedef FMacPlatformMisc FPlatformMisc;

enum EMacModifierKeys
{
	MMK_RightCommand	= 0xF754,
	MMK_LeftCommand		= 0xF755,
	MMK_LeftShift		= 0xF756,
	MMK_CapsLock		= 0xF757,
	MMK_LeftAlt			= 0xF758,
	MMK_LeftControl		= 0xF759,
	MMK_RightShift		= 0xF760,
	MMK_RightAlt		= 0xF761,
	MMK_RightControl	= 0xF762
};
