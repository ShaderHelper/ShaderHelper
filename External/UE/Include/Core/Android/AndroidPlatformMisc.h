// Copyright Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	AndroidMisc.h: Android platform misc functions
==============================================================================================*/

#pragma once
#include "CoreTypes.h"
#include "Android/AndroidSystemIncludes.h"
#include "GenericPlatform/GenericPlatformMisc.h"
//@todo android: this entire file

template <typename FuncType>
class TFunction;


#define PLATFORM_BREAK()	raise(SIGTRAP)

#define UE_DEBUG_BREAK_IMPL()	PLATFORM_BREAK()

#define ANDROID_HAS_RTSIGNALS PLATFORM_USED_NDK_VERSION_INTEGER >= 21

enum class ECrashContextType;

/**
 * Android implementation of the misc OS functions
 */
struct CORE_API FAndroidMisc : public FGenericPlatformMisc
{
	static void RequestExit( bool Force );
	static bool RestartApplication();
	static void LocalPrint(const TCHAR *Message);
	static bool IsLocalPrintThreadSafe() { return true; }
	static void PlatformPreInit();
	static void PlatformInit();
	static void PlatformTearDown();
	static void PlatformHandleSplashScreen(bool ShowSplashScreen);
    static EDeviceScreenOrientation GetDeviceOrientation() { return DeviceOrientation; }
	static void SetDeviceOrientation(EDeviceScreenOrientation NewDeviceOrentation);
    
	FORCEINLINE static int32 GetMaxPathLength()
	{
		return ANDROID_MAX_PATH;
	}

	UE_DEPRECATED(4.21, "void FPlatformMisc::GetEnvironmentVariable(Name, Result, Length) is deprecated. Use FString FPlatformMisc::GetEnvironmentVariable(Name) instead.")
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength);

	static FString GetEnvironmentVariable(const TCHAR* VariableName);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );
	static bool UseRenderThread();
	static bool HasPlatformFeature(const TCHAR* FeatureName);
	static bool ShouldDisablePluginAtRuntime(const FString& PluginName);
	static bool SupportsES30();

public:

	static bool AllowThreadHeartBeat()
	{
		return false;
	}

	struct FCPUStatTime{
		uint64_t			TotalTime;
		uint64_t			UserTime;
		uint64_t			NiceTime;
		uint64_t			SystemTime;
		uint64_t			SoftIRQTime;
		uint64_t			IRQTime;
		uint64_t			IdleTime;
		uint64_t			IOWaitTime;
	};

	struct FCPUState
	{
		const static int32			MaxSupportedCores = 16; //Core count 16 is maximum for now
		int32						CoreCount;
		int32						ActivatedCoreCount;
		ANSICHAR					Name[6];
		FAndroidMisc::FCPUStatTime	CurrentUsage[MaxSupportedCores]; 
		FAndroidMisc::FCPUStatTime	PreviousUsage[MaxSupportedCores];
		int32						Status[MaxSupportedCores];
		double						Utilization[MaxSupportedCores];
		double						AverageUtilization;
		
	};

	static FCPUState& GetCPUState();
	static int32 NumberOfCores();
	static int32 NumberOfCoresIncludingHyperthreads();
	static bool SupportsLocalCaching();
	static void CreateGuid(struct FGuid& Result);
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext& Context));
	// NOTE: THIS FUNCTION IS DEFINED IN ANDROIDOPENGL.CPP
	static void GetValidTargetPlatforms(class TArray<class FString>& TargetPlatformNames);
	static bool GetUseVirtualJoysticks();
	static bool SupportsTouchInput();
	static const TCHAR* GetDefaultDeviceProfileName() { return TEXT("Android_Default"); }
	static bool GetVolumeButtonsHandledBySystem();
	static void SetVolumeButtonsHandledBySystem(bool enabled);
	// Returns current volume, 0-15
	static int GetVolumeState(double* OutTimeOfChangeInSec = nullptr);

	static int32 GetDeviceVolume();

#if USE_ANDROID_FILE
	static const TCHAR* GamePersistentDownloadDir();
	static FString GetLoginId();
#endif
#if USE_ANDROID_JNI
	static FString GetDeviceId();
	static FString GetUniqueAdvertisingId();
#endif
	static FString GetCPUVendor();
	static FString GetCPUBrand();
	static FString GetCPUChipset();
	static FString GetPrimaryGPUBrand();
	static void GetOSVersions(FString& out_OSVersionLabel, FString& out_OSSubVersionLabel);
	static bool GetDiskTotalAndFreeSpace(const FString& InPath, uint64& TotalNumberOfBytes, uint64& NumberOfFreeBytes);
	
	enum EBatteryState
	{
		BATTERY_STATE_UNKNOWN = 1,
		BATTERY_STATE_CHARGING,
		BATTERY_STATE_DISCHARGING,
		BATTERY_STATE_NOT_CHARGING,
		BATTERY_STATE_FULL
	};
	struct FBatteryState
	{
		FAndroidMisc::EBatteryState	State;
		int							Level;          // in range [0,100]
		float						Temperature;    // in degrees of Celsius
	};

	static FBatteryState GetBatteryState();
	static int GetBatteryLevel();
	static bool IsRunningOnBattery();
	static bool IsInLowPowerMode();
	static float GetDeviceTemperatureLevel();
	static bool AreHeadPhonesPluggedIn();
	static ENetworkConnectionType GetNetworkConnectionType();
#if USE_ANDROID_JNI
	static bool HasActiveWiFiConnection();
#endif

	static void RegisterForRemoteNotifications();
	static void UnregisterForRemoteNotifications();
	static bool IsAllowedRemoteNotifications();

	/** @return Memory representing a true type or open type font provided by the platform as a default font for unreal to consume; empty array if the default font failed to load. */
	static TArray<uint8> GetSystemFontBytes();

	static IPlatformChunkInstall* GetPlatformChunkInstall();

	static void PrepareMobileHaptics(EMobileHapticsType Type);
	static void TriggerMobileHaptics();
	static void ReleaseMobileHaptics();
	static void ShareURL(const FString& URL, const FText& Description, int32 LocationHintX, int32 LocationHintY);

	static FString LoadTextFileFromPlatformPackage(const FString& RelativePath);
	static bool FileExistsInPlatformPackage(const FString& RelativePath);

	// ANDROID ONLY:
	static void SetVersionInfo(FString AndroidVersion, int32 InTargetSDKVersion, FString DeviceMake, FString DeviceModel, FString DeviceBuildNumber, FString OSLanguage);
	static const FString GetAndroidVersion();
	static int32 GetAndroidMajorVersion();
	static int32 GetTargetSDKVersion();
	static const FString GetDeviceMake();
	static const FString GetDeviceModel();
	static const FString GetOSLanguage();
	static const FString GetDeviceBuildNumber();
	static const FString GetProjectVersion();
	static FString GetDefaultLocale();
	static FString GetGPUFamily();
	static FString GetGLVersion();
	static bool SupportsFloatingPointRenderTargets();
	static bool SupportsShaderFramebufferFetch();
	static bool SupportsShaderIOBlocks();
#if USE_ANDROID_JNI
	static int GetAndroidBuildVersion();
#endif
	static bool IsSupportedAndroidDevice();
	static void SetForceUnsupported(bool bInOverride);
	static TMap<FString, FString> GetConfigRulesTMap();
	static FString* GetConfigRulesVariable(const FString& Key);

	/* HasVulkanDriverSupport
	 * @return true if this Android device supports a Vulkan API Unreal could use
	 */
	static bool HasVulkanDriverSupport();

	/* IsVulkanAvailable
	 * @return	true if there is driver support, we have an RHI, we are packaged with Vulkan support,
	 *			and not we are not forcing GLES with a command line switch
	 */
	static bool IsVulkanAvailable();

	/* ShouldUseVulkan
	 * @return true if Vulkan is available, and not disabled by device profile cvar
	 */
	static bool ShouldUseVulkan();
	static bool ShouldUseDesktopVulkan();
	static FString GetVulkanVersion();
	typedef TFunction<void(void* NewNativeHandle)> ReInitWindowCallbackType;
	static ReInitWindowCallbackType GetOnReInitWindowCallback();
	static void SetOnReInitWindowCallback(ReInitWindowCallbackType InOnReInitWindowCallback);
	typedef TFunction<void()> ReleaseWindowCallbackType;
	static ReleaseWindowCallbackType GetOnReleaseWindowCallback();
	static void SetOnReleaseWindowCallback(ReleaseWindowCallbackType InOnReleaseWindowCallback);
	static FString GetOSVersion();
	static bool GetOverrideResolution(int32 &ResX, int32& ResY) { return false; }
	typedef TFunction<void()> OnPauseCallBackType;
	static OnPauseCallBackType GetOnPauseCallback();
	static void SetOnPauseCallback(OnPauseCallBackType InOnPauseCallback);
	static void TriggerCrashHandler(ECrashContextType InType, const TCHAR* InErrorMessage, const TCHAR* OverrideCallstack = nullptr);

	// To help track down issues with failing crash handler.
	static FString GetFatalSignalMessage(int Signal, siginfo* Info);
	static void OverrideFatalSignalHandler(void (*FatalSignalHandlerOverrideFunc)(int Signal, struct siginfo* Info, void* Context));
	// To help track down issues with failing crash handler.

	static bool IsInSignalHandler();

#if !UE_BUILD_SHIPPING
	static bool IsDebuggerPresent();
#endif

	FORCEINLINE static void MemoryBarrier()
	{
		__sync_synchronize();
	}


#if STATS || ENABLE_STATNAMEDEVENTS
	static void BeginNamedEventFrame();
	static void BeginNamedEvent(const struct FColor& Color, const TCHAR* Text);
	static void BeginNamedEvent(const struct FColor& Color, const ANSICHAR* Text);
	static void EndNamedEvent();
	static void CustomNamedStat(const TCHAR* Text, float Value, const TCHAR* Graph, const TCHAR* Unit);
	static void CustomNamedStat(const ANSICHAR* Text, float Value, const ANSICHAR* Graph, const ANSICHAR* Unit);
#endif

#if (STATS || ENABLE_STATNAMEDEVENTS)
	static int32 TraceMarkerFileDescriptor;
#endif
	
	// run time compatibility information
	static FString AndroidVersion; // version of android we are running eg "4.0.4"
	static int32 AndroidMajorVersion; // integer major version of Android we are running, eg 10
	static int32 TargetSDKVersion; // Target SDK version, eg 29.
	static FString DeviceMake; // make of the device we are running on eg. "samsung"
	static FString DeviceModel; // model of the device we are running on eg "SAMSUNG-SGH-I437"
	static FString DeviceBuildNumber; // platform image build number of device "R16NW.G960NKSU1ARD6"
	static FString OSLanguage; // language code the device is set to

	// Build version of Android, i.e. API level.
	static int32 AndroidBuildVersion;

	// Key/Value pair variables from the optional configuration.txt
	static TMap<FString, FString> ConfigRulesVariables;

	static bool VolumeButtonsHandledBySystem;

	static bool bNeedsRestartAfterPSOPrecompile;

	enum class ECoreFrequencyProperty
	{
		CurrentFrequency,
		MaxFrequency,
		MinFrequency,
	};

	static uint32 GetCoreFrequency(int32 CoreIndex, ECoreFrequencyProperty CoreFrequencyProperty);

	// Returns CPU temperature read from one of the configurable CPU sensors via android.CPUThermalSensorFilePath CVar or AndroidEngine.ini, [ThermalSensors] section.
	// Doesn't guarantee to work on all devices. Some devices require root access rights to read sensors information, in that case 0.0 will be returned
	static float GetCPUTemperature();

	static void SaveDeviceOrientation(EDeviceScreenOrientation NewDeviceOrentation) { DeviceOrientation = NewDeviceOrentation; }

	// Window access is locked by the game thread before preinit and unlocked here after RHIInit (PlatformCreateDynamicRHI). 
	static void UnlockAndroidWindow();
	
	static TArray<int32> GetSupportedNativeDisplayRefreshRates();

	static bool SetNativeDisplayRefreshRate(int32 RefreshRate);

	static int32 GetNativeDisplayRefreshRate();

	/**
	 * Returns whether or not a 16 bit index buffer should be promoted to 32 bit on load, needed for some Android devices
	 */
	static bool Expand16BitIndicesTo32BitOnLoad();

	/**
	 * Will return true if we wish to propagate the alpha to the backbuffer
	 */
	static int GetMobilePropagateAlphaSetting();

	static bool SupportsBackbufferSampling();

	static void SetMemoryWarningHandler(void (*Handler)(const FGenericMemoryWarningContext& Context));
	static bool HasMemoryWarningHandler();

	// Android specific requesting of exit, *ONLY* use this function in signal handling code. Otherwise normal RequestExit functions
	static void NonReentrantRequestExit();

	// Register/Get thread names for Android specific threads
	static void RegisterThreadName(const char* Name, uint32 ThreadId);
	static const char* GetThreadName(uint32 ThreadId);

	static void ShowConsoleWindow();

private:
	static const ANSICHAR* CodeToString(int Signal, int si_code);
	static EDeviceScreenOrientation DeviceOrientation;

#if USE_ANDROID_JNI
	enum class EAndroidScreenOrientation
	{
		SCREEN_ORIENTATION_UNSPECIFIED = -1,
		SCREEN_ORIENTATION_LANDSCAPE = 0,
		SCREEN_ORIENTATION_PORTRAIT = 1,
		SCREEN_ORIENTATION_USER = 2,
		SCREEN_ORIENTATION_BEHIND = 3,
		SCREEN_ORIENTATION_SENSOR = 4,
		SCREEN_ORIENTATION_NOSENSOR = 5,
		SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 6,
		SCREEN_ORIENTATION_SENSOR_PORTRAIT = 7,
		SCREEN_ORIENTATION_REVERSE_LANDSCAPE = 8,
		SCREEN_ORIENTATION_REVERSE_PORTRAIT = 9,
		SCREEN_ORIENTATION_FULL_SENSOR = 10,
		SCREEN_ORIENTATION_USER_LANDSCAPE = 11,
		SCREEN_ORIENTATION_USER_PORTRAIT = 12,
	};
	
	static int32 GetAndroidScreenOrientation(EDeviceScreenOrientation ScreenOrientation);
#endif // USE_ANDROID_JNI
};

typedef FAndroidMisc FPlatformMisc;
