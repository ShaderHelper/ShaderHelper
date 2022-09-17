// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "Delegates/Delegate.h"
#include "Math/IntVector.h"
#include "Misc/AES.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Async/TaskGraphInterfaces.h"

class AActor;
class Error;
class IPakFile;

// delegates for hotfixes
namespace EHotfixDelegates
{
	enum Type
	{
		Test,
	};
}


// this is an example of a hotfix arg and return value structure. Once we have other examples, it can be deleted.
struct FTestHotFixPayload
{
	FString Message;
	bool ValueToReturn;
	bool Result;
};

// Parameters passed to CrashOverrideParamsChanged used to customize crash report client behavior/appearance. If the corresponding bool is not true, this value will not be stored.
struct FCrashOverrideParameters
{
	UE_DEPRECATED(4.21, "CrashReportClientMessageText should now be set through the CrashReportClientRichText property in the [CrashContextProperties] section of DefaultEngine.ini.")
	FString CrashReportClientMessageText;
	/** Appended to the end of GameName (which is retreived from FApp::GetGameName). */
	FString GameNameSuffix;
	/** Default this to true for backward compatibility before these bools were added. */
	bool bSetCrashReportClientMessageText = true;
	bool bSetGameNameSuffix = false;
	TOptional<bool> SendUnattendedBugReports;
	TOptional<bool> SendUsageData;

	CORE_API ~FCrashOverrideParameters();
};

class CORE_API FCoreDelegates
{
public:
	//hot fix delegate
	DECLARE_DELEGATE_TwoParams(FHotFixDelegate, void *, int32);

	// Callback for object property modifications
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorLabelChanged, AActor*);

	// delegate type for prompting the pak system to mount all pak files, which haven't already been mounted, from all default locations
	DECLARE_DELEGATE_RetVal_OneParam(int32, FOnMountAllPakFiles, const TArray<FString>&);

	// deprecated delegate type for prompting the pak system to mount a new pak
	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnMountPak, const FString&, int32, IPlatformFile::FDirectoryVisitor*);

	// delegate type for prompting the pak system to mount a new pak
	DECLARE_DELEGATE_RetVal_TwoParams(IPakFile*, FMountPak, const FString&, int32);

	// delegate type for prompting the pak system to unmount a pak
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnUnmountPak, const FString&);

	// delegate type for prompting the pak system to optimize memory for mounted paks
	DECLARE_DELEGATE(FOnOptimizeMemoryUsageForMountedPaks);

	// deprecated delegate for handling when a new pak file is successfully mounted passes in the name of the mounted pak file
	DECLARE_MULTICAST_DELEGATE_OneParam(FPakFileMountedDelegate, const TCHAR*);

	// deprecated delegate for handling when a new pak file is successfully mounted passes in the name of the pak file and its chunk ID (or INDEX_NONE)
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPakFileMounted, const TCHAR*, const int32);

	// delegate for handling when a new pak file is successfully mounted
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPakFileMounted2, const IPakFile&);

	// delegate to let other systems no that no paks were mounted, in case something wants to handle that case
	DECLARE_MULTICAST_DELEGATE(FNoPakFilesMountedDelegate);

	/** delegate type for opening a modal message box ( Params: EAppMsgType::Type MessageType, const FText& Text, const FText& Title ) */
	DECLARE_DELEGATE_RetVal_ThreeParams(EAppReturnType::Type, FOnModalMessageBox, EAppMsgType::Type, const FText&, const FText&);

	// Callback for handling an ensure
	DECLARE_MULTICAST_DELEGATE(FOnHandleSystemEnsure);

	// Callback for handling an error
	DECLARE_MULTICAST_DELEGATE(FOnHandleSystemError);

	typedef TSharedPtr<class IMovieStreamer, ESPMode::ThreadSafe> FMovieStreamerPtr;
    // Delegate used to register a movie streamer with any movie player modules that bind to this delegate
    DECLARE_MULTICAST_DELEGATE_OneParam(FRegisterMovieStreamerDelegate, FMovieStreamerPtr);

    // Delegate used to un-register a movie streamer with any movie player modules that bind to this delegate
    DECLARE_MULTICAST_DELEGATE_OneParam(FUnRegisterMovieStreamerDelegate, FMovieStreamerPtr);

	// Callback for handling user login/logout.  first int is UserID, second int is UserIndex
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnUserLoginChangedEvent, bool, int32, int32);

	// Callback for handling safe frame area size changes
	DECLARE_MULTICAST_DELEGATE(FOnSafeFrameChangedEvent);

	// Callback for handling accepting invitations - generally for engine code
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInviteAccepted, const FString&, const FString&);

	UE_DEPRECATED(4.26, "FCoreDelegates::FRegisterEncryptionKeyDelegate is deprecated; use FRegisterEncryptionKeyMulticastDelegate instead")
	DECLARE_DELEGATE_TwoParams(FRegisterEncryptionKeyDelegate, const FGuid&, const FAES::FAESKey&);

	// Callback for registering a new encryption key
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRegisterEncryptionKeyMulticastDelegate, const FGuid&, const FAES::FAESKey&);

	// Callback for accessing pak encryption key, if it exists
	DECLARE_DELEGATE_OneParam(FPakEncryptionKeyDelegate, uint8[32]);

	// Callback for gathering pak signing keys, if they exist
	DECLARE_DELEGATE_TwoParams(FPakSigningKeysDelegate, TArray<uint8>&, TArray<uint8>&);

	// Callback for handling the Controller connection / disconnection
	// first param is true for a connection, false for a disconnection.
	// second param is UserID, third is UserIndex / ControllerId.
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnUserControllerConnectionChange, bool, FPlatformUserId, int32);

	// Callback for handling a Controller pairing change
	// first param is controller index
	// second param is NewUserPlatformId, third is OldUserPlatformId.
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnUserControllerPairingChange, int32 /*ControllerIndex*/, FPlatformUserId /*NewUserPlatformId*/, FPlatformUserId /*OldUserPlatformId*/);

	// Callback for platform handling when flushing async loads.
	DECLARE_MULTICAST_DELEGATE(FOnAsyncLoadingFlush);
	static FOnAsyncLoadingFlush OnAsyncLoadingFlush;

	// Callback for a game thread interruption point when a async load flushing. Used to updating UI during long loads.
	DECLARE_MULTICAST_DELEGATE(FOnAsyncLoadingFlushUpdate);
	static FOnAsyncLoadingFlushUpdate OnAsyncLoadingFlushUpdate;

	// Callback on the game thread when an async load is started. This goes off before the packages has finished loading
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAsyncLoadPackage, const FString&);
	static FOnAsyncLoadPackage OnAsyncLoadPackage;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSyncLoadPackage, const FString&);
	static FOnSyncLoadPackage OnSyncLoadPackage;

	// get a hotfix delegate
	static FHotFixDelegate& GetHotfixDelegate(EHotfixDelegates::Type HotFix);

	// Callback when a user logs in/out of the platform.
	static FOnUserLoginChangedEvent OnUserLoginChangedEvent;

	// Callback when controllers disconnected / reconnected
	static FOnUserControllerConnectionChange OnControllerConnectionChange;

	// Callback when a single controller pairing changes
	static FOnUserControllerPairingChange OnControllerPairingChange;

	// Callback when a user changes the safe frame size
	static FOnSafeFrameChangedEvent OnSafeFrameChangedEvent;

	// Callback for mounting all the pak files in default locations
	static FOnMountAllPakFiles OnMountAllPakFiles;

	// Callback to prompt the pak system to mount a pak file
	static FMountPak MountPak;

	UE_DEPRECATED(4.26, "OnMountPak is deprecated; use MountPak instead.")
	static FOnMountPak OnMountPak;

	// Callback to prompt the pak system to unmount a pak file.
	static FOnUnmountPak OnUnmountPak;

	// Callback to optimize memeory for currently mounted paks
	static FOnOptimizeMemoryUsageForMountedPaks OnOptimizeMemoryUsageForMountedPaks;

	// After a pakfile is mounted this is called
	static FOnPakFileMounted2 OnPakFileMounted2;

	UE_DEPRECATED(4.26, "FCoreDelegates::OnPakFileMounted is deprecated; use OnPakFileMounted2 instead")
	static FOnPakFileMounted OnPakFileMounted;

	UE_DEPRECATED(4.25, "FCoreDelegates::PakFileMountedCallback is deprecated. Use FCoreDelegates::OnPakFileMounted2 instead.")
	static FPakFileMountedDelegate PakFileMountedCallback;

	// After a file is added this is called
	DECLARE_MULTICAST_DELEGATE_OneParam(FNewFileAddedDelegate, const FString&);
	static FNewFileAddedDelegate NewFileAddedDelegate;

	// After an attempt to mount all pak files, but none wre found, this is called
	static FNoPakFilesMountedDelegate NoPakFilesMountedDelegate;

	// When a file is opened for read from a pak file
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFileOpenedForReadFromPakFile, const TCHAR* /*PakFile*/, const TCHAR* /*FileName*/);
	static FOnFileOpenedForReadFromPakFile OnFileOpenedForReadFromPakFile;

    // Delegate used to register a movie streamer with any movie player modules that bind to this delegate
    // Designed to be called when a platform specific movie streamer plugin starts up so that it doesn't need to implement a register for all movie player plugins
    static FRegisterMovieStreamerDelegate RegisterMovieStreamerDelegate;
    // Delegate used to un-register a movie streamer with any movie player modules that bind to this delegate
    // Designed to be called when a platform specific movie streamer plugin shuts down so that it doesn't need to implement a register for all movie player plugins
    static FUnRegisterMovieStreamerDelegate UnRegisterMovieStreamerDelegate;

	// Callback when an ensure has occurred
	static FOnHandleSystemEnsure OnHandleSystemEnsure;

	// Callback when an error (crash) has occurred
	static FOnHandleSystemError OnHandleSystemError;

	// Called when an actor label is changed
	static FOnActorLabelChanged OnActorLabelChanged;

	UE_DEPRECATED(4.26, "FCoreDelegates::GetRegisterEncryptionKeyDelegate is deprecated; use GetRegisterEncryptionKeyMulticastDelegate instead")
	PRAGMA_DISABLE_DEPRECATION_WARNINGS // Don't warn about FRegisterEncryptionKeyDelegate
	static FRegisterEncryptionKeyDelegate& GetRegisterEncryptionKeyDelegate();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	static FRegisterEncryptionKeyMulticastDelegate& GetRegisterEncryptionKeyMulticastDelegate();
	static FPakEncryptionKeyDelegate& GetPakEncryptionKeyDelegate();
	static FPakSigningKeysDelegate& GetPakSigningKeysDelegate();

	

#if WITH_EDITOR
	// Called before the editor displays a modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
	static FSimpleMulticastDelegate PreModal;

	// Called after the editor dismisses a modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
	static FSimpleMulticastDelegate PostModal;
    
    // Called before the editor displays a Slate (non-platform) modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
    static FSimpleMulticastDelegate PreSlateModal;
    
    // Called after the editor dismisses a Slate (non-platform) modal window, allowing other windows the opportunity to disable themselves to avoid reentrant calls
    static FSimpleMulticastDelegate PostSlateModal;
    
#endif	//WITH_EDITOR
	
	// Called when an error occurred.
	static FSimpleMulticastDelegate OnShutdownAfterError;

	// Called when appInit is called, very early in startup
	static FSimpleMulticastDelegate OnInit;

	// Called at the end of UEngine::Init, right before loading PostEngineInit modules for both normal execution and commandlets
	static FSimpleMulticastDelegate OnPostEngineInit;

	// Called after all modules have been loaded for all phases
	static FSimpleMulticastDelegate OnAllModuleLoadingPhasesComplete;

	// Called at the very end of engine initialization, right before the engine starts ticking. This is not called for commandlets
	static FSimpleMulticastDelegate OnFEngineLoopInitComplete;

	// Called when the application is about to exit.
	static FSimpleMulticastDelegate OnExit;

	// Called when before the application is exiting.
	static FSimpleMulticastDelegate OnPreExit;

	// Called before the engine exits. Separate from OnPreExit as OnEnginePreExit occurs before shutting down any core modules.
	static FSimpleMulticastDelegate OnEnginePreExit;

	/** Delegate for gathering up additional localization paths that are unknown to the engine core (such as plugins) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FGatherAdditionalLocResPathsDelegate, TArray<FString>&);
	static FGatherAdditionalLocResPathsDelegate GatherAdditionalLocResPathsCallback;

	/** Color picker color has changed, please refresh as needed*/
	static FSimpleMulticastDelegate ColorPickerChanged;

	/** requests to open a message box */
	static FOnModalMessageBox ModalErrorMessage;

	/** Called when the user accepts an invitation to the current game */
	static FOnInviteAccepted OnInviteAccepted;

	// Called at the beginning of a frame
	static FSimpleMulticastDelegate OnBeginFrame;

	// Called at the moment of sampling the input (currently on the gamethread)
	static FSimpleMulticastDelegate OnSamplingInput;

	// Called at the end of a frame
	static FSimpleMulticastDelegate OnEndFrame;

	// Called at the beginning of a frame on the renderthread
	static FSimpleMulticastDelegate OnBeginFrameRT;

	// Called at the end of a frame on the renderthread
	static FSimpleMulticastDelegate OnEndFrameRT;


	DECLARE_MULTICAST_DELEGATE_ThreeParams(FWorldOriginOffset, class UWorld*, FIntVector, FIntVector);
	/** called before world origin shifting */
	static FWorldOriginOffset PreWorldOriginOffset;
	/** called after world origin shifting */
	static FWorldOriginOffset PostWorldOriginOffset;

	/** called when the main loop would otherwise starve. */
	DECLARE_DELEGATE(FStarvedGameLoop);
	static FStarvedGameLoop StarvedGameLoop;

	// IOS-style temperature updates, allowing game to scale down to let temp drop (to avoid thermal throttling on mobile, for instance) */
	// There is a parellel enum in ApplicationLifecycleComponent
	enum class ETemperatureSeverity : uint8
	{
		Unknown,
		Good,
		Bad,
		Serious,
		Critical,

		NumSeverities,
	};
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTemperatureChange, ETemperatureSeverity);
	static FOnTemperatureChange OnTemperatureChange;

	/** Called when the OS goes into low power mode */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLowPowerMode, bool);
	static FOnLowPowerMode OnLowPowerMode;



	DECLARE_MULTICAST_DELEGATE_TwoParams(FCountPreLoadConfigFileRespondersDelegate, const TCHAR* /*IniFilename*/, int32& /*ResponderCount*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FPreLoadConfigFileDelegate, const TCHAR* /*IniFilename*/, FString& /*LoadedContents*/);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FPreSaveConfigFileDelegate, const TCHAR* /*IniFilename*/, const FString& /*ContentsToSave*/, int32& /*SavedCount*/);
	static FCountPreLoadConfigFileRespondersDelegate CountPreLoadConfigFileRespondersDelegate;
	static FPreLoadConfigFileDelegate PreLoadConfigFileDelegate;
	static FPreSaveConfigFileDelegate PreSaveConfigFileDelegate;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFConfigFileCreated, const FConfigFile *);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFConfigFileDeleted, const FConfigFile *);
	static FOnFConfigFileCreated OnFConfigCreated;
	static FOnFConfigFileDeleted OnFConfigDeleted;

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnConfigValueRead, const TCHAR* /*IniFilename*/, const TCHAR* /*SectionName*/, const TCHAR* /*Key*/);
	static FOnConfigValueRead OnConfigValueRead;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnConfigSectionRead, const TCHAR* /*IniFilename*/, const TCHAR* /*SectionName*/);
	static FOnConfigSectionRead OnConfigSectionRead;
	static FOnConfigSectionRead OnConfigSectionNameRead;

	DECLARE_MULTICAST_DELEGATE_FourParams(FOnApplyCVarFromIni, const TCHAR* /*SectionName*/, const TCHAR* /*IniFilename*/, uint32 /*SetBy*/, bool /*bAllowCheating*/);
	static FOnApplyCVarFromIni OnApplyCVarFromIni;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSystemResolutionChanged, uint32 /*ResX*/, uint32 /*ResY*/);
	static FOnSystemResolutionChanged OnSystemResolutionChanged;

#if WITH_EDITOR
	// called when a target platform changes it's return value of supported formats.  This is so anything caching those results can reset (like cached shaders for cooking)
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetPlatformChangedSupportedFormats, const ITargetPlatform*); 
	static FOnTargetPlatformChangedSupportedFormats OnTargetPlatformChangedSupportedFormats;

	// Called when a feature level is disabled by the user.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFeatureLevelDisabled, int, const FName&);
	static FOnFeatureLevelDisabled OnFeatureLevelDisabled;
#endif

	/** IOS-style application lifecycle delegates */
	DECLARE_MULTICAST_DELEGATE(FApplicationLifetimeDelegate);

	// This is called when the application is about to be deactivated (e.g., due to a phone call or SMS or the sleep button).
	// The game should be paused if possible, etc...
	static FApplicationLifetimeDelegate ApplicationWillDeactivateDelegate;

	// Called when the application has been reactivated (reverse any processing done in the Deactivate delegate)
	static FApplicationLifetimeDelegate ApplicationHasReactivatedDelegate;

	// This is called when the application is being backgrounded (e.g., due to switching
	// to another app or closing it via the home button)
	// The game should release shared resources, save state, etc..., since it can be
	// terminated from the background state without any further warning.
	static FApplicationLifetimeDelegate ApplicationWillEnterBackgroundDelegate; // for instance, hitting the home button

	// Called when the application is returning to the foreground (reverse any processing done in the EnterBackground delegate)
	static FApplicationLifetimeDelegate ApplicationHasEnteredForegroundDelegate;

	// This *may* be called when the application is getting terminated by the OS.
	// There is no guarantee that this will ever be called on a mobile device,
	// save state when ApplicationWillEnterBackgroundDelegate is called instead.
	static FApplicationLifetimeDelegate ApplicationWillTerminateDelegate;

	// Called when in the background, if the OS is giving CPU time to the device. It is very likely
	// this will never be called due to mobile OS backgrounded CPU restrictions. But if, for instance,
	// VOIP is active on iOS, the will be getting called
	DECLARE_MULTICAST_DELEGATE_OneParam(FBackgroundTickDelegate, float /*DeltaTime*/);
	static FBackgroundTickDelegate MobileBackgroundTickDelegate;

	// Called when the OS needs control of the music (parameter is true) or when the OS returns
	// control of the music to the application (parameter is false). This can happen due to a
	// phone call or timer or other OS-level event. This is currently triggered only on iOS
	// devices.
	DECLARE_MULTICAST_DELEGATE_OneParam(FUserMusicInterruptDelegate, bool);
	static FUserMusicInterruptDelegate UserMusicInterruptDelegate;
	
	// [iOS only] Called when the mute switch is detected as changed or when the
	// volume changes. Parameter 1 is the mute switch state (true is muted, false is
	// unmuted). Parameter 2 is the volume as an integer from 0 to 100.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FAudioMuteDelegate, bool, int);
	static FAudioMuteDelegate AudioMuteDelegate;
	
	// [iOS only] Called when the audio device changes
	// For instance, when the headphones are plugged in or removed
	DECLARE_MULTICAST_DELEGATE_OneParam(FAudioRouteChangedDelegate, bool);
	static FAudioRouteChangedDelegate AudioRouteChangedDelegate;

	// Generally, events triggering UserMusicInterruptDelegate or AudioMuteDelegate happen only
	// when a change occurs. When a system comes online needing the current audio state but the
	// event has already been broadcast, calling ApplicationRequestAudioState will force the
	// UserMusicInterruptDelegate and AudioMuteDelegate to be called again if the low-level
	// application layer supports it. Currently, this is available only on iOS.
	DECLARE_MULTICAST_DELEGATE(FApplicationRequestAudioState);
	static FApplicationRequestAudioState ApplicationRequestAudioState;
	
	// Called when the OS is running low on resources and asks the application to free up any cached resources, drop graphics quality etc.
	static FApplicationLifetimeDelegate ApplicationShouldUnloadResourcesDelegate;

	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationStartupArgumentsDelegate, const TArray<FString>&);

	// Called with arguments passed to the application on statup, perhaps meta data passed on by another application which launched this one.
	static FApplicationStartupArgumentsDelegate ApplicationReceivedStartupArgumentsDelegate;

	/** IOS-style push notification delegates */
	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationRegisteredForRemoteNotificationsDelegate, TArray<uint8>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationRegisteredForUserNotificationsDelegate, int);
	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationFailedToRegisterForRemoteNotificationsDelegate, FString);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FApplicationReceivedRemoteNotificationDelegate, FString, int);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FApplicationReceivedLocalNotificationDelegate, FString, int, int);
    DECLARE_MULTICAST_DELEGATE(FApplicationPerformFetchDelegate);
    DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationBackgroundSessionEventDelegate, FString);

	// called when the user grants permission to register for remote notifications
	static FApplicationRegisteredForRemoteNotificationsDelegate ApplicationRegisteredForRemoteNotificationsDelegate;

	// called when the user grants permission to register for notifications
	static FApplicationRegisteredForUserNotificationsDelegate ApplicationRegisteredForUserNotificationsDelegate;

	// called when the application fails to register for remote notifications
	static FApplicationFailedToRegisterForRemoteNotificationsDelegate ApplicationFailedToRegisterForRemoteNotificationsDelegate;

	// called when the application receives a remote notification
	static FApplicationReceivedRemoteNotificationDelegate ApplicationReceivedRemoteNotificationDelegate;

	// called when the application receives a local notification
	static FApplicationReceivedLocalNotificationDelegate ApplicationReceivedLocalNotificationDelegate;

    // called when the application receives notice to perform a background fetch
    static FApplicationPerformFetchDelegate ApplicationPerformFetchDelegate;

    // called when the application receives notice that a background download has completed
    static FApplicationBackgroundSessionEventDelegate ApplicationBackgroundSessionEventDelegate;

	/** Sent when a device screen orientation changes */
	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationReceivedOnScreenOrientationChangedNotificationDelegate, int32);
	static FApplicationReceivedOnScreenOrientationChangedNotificationDelegate ApplicationReceivedScreenOrientationChangedNotificationDelegate;

	/** Checks to see if the stat is already enabled */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FStatCheckEnabled, const TCHAR*, bool&, bool&);
	static FStatCheckEnabled StatCheckEnabled;

	/** Sent after each stat is enabled */
	DECLARE_MULTICAST_DELEGATE_OneParam(FStatEnabled, const TCHAR*);
	static FStatEnabled StatEnabled;

	/** Sent after each stat is disabled */
	DECLARE_MULTICAST_DELEGATE_OneParam(FStatDisabled, const TCHAR*);
	static FStatDisabled StatDisabled;

	/** Sent when all stats need to be disabled */
	DECLARE_MULTICAST_DELEGATE_OneParam(FStatDisableAll, const bool);
	static FStatDisableAll StatDisableAll;

	// Called when an application is notified that the application license info has been updated.
	// The new license data should be polled and steps taken based on the results (i.e. halt application if license is no longer valid).
	DECLARE_MULTICAST_DELEGATE(FApplicationLicenseChange);
	static FApplicationLicenseChange ApplicationLicenseChange;

	/** Sent when the platform changed its laptop mode (for convertible laptops).*/
	DECLARE_MULTICAST_DELEGATE_OneParam(FPlatformChangedLaptopMode, EConvertibleLaptopMode);
	static FPlatformChangedLaptopMode PlatformChangedLaptopMode;

	/** Sent when the platform needs the user to fix headset tracking on startup (Most platforms do not need this.) */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetTrackingInitializingAndNeedsHMDToBeTrackedDelegate);
	static FVRHeadsetTrackingInitializingAndNeedsHMDToBeTrackedDelegate VRHeadsetTrackingInitializingAndNeedsHMDToBeTrackedDelegate;

	/** Sent when the platform finds that needed headset tracking on startup has completed (Most platforms do not need this.) */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetTrackingInitializedDelegate);
	static FVRHeadsetTrackingInitializedDelegate VRHeadsetTrackingInitializedDelegate;

	/** Sent when the platform requests a low-level VR recentering */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetRecenter);
	static FVRHeadsetRecenter VRHeadsetRecenter;

	/** Sent when connection to VR HMD is lost */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetLost);
	static FVRHeadsetLost VRHeadsetLost;

	/** Sent when connection to VR HMD is restored */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetReconnected);
	static FVRHeadsetReconnected VRHeadsetReconnected;

	/** Sent when connection to VR HMD connection is refused by the player */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetConnectCanceled);
	static FVRHeadsetConnectCanceled VRHeadsetConnectCanceled;

	/** Sent when the VR HMD detects that it has been put on by the player. */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetPutOnHead);
	static FVRHeadsetPutOnHead VRHeadsetPutOnHead;

	/** Sent when the VR HMD detects that it has been taken off by the player. */
	DECLARE_MULTICAST_DELEGATE(FVRHeadsetRemovedFromHead);
	static FVRHeadsetRemovedFromHead VRHeadsetRemovedFromHead;

	/** Sent when a 3DOF VR controller is recentered */
	DECLARE_MULTICAST_DELEGATE(FVRControllerRecentered);
	static FVRControllerRecentered VRControllerRecentered;

	/** Sent when application code changes the user activity hint string for analytics, crash reports, etc */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUserActivityStringChanged, const FString&);
	static FOnUserActivityStringChanged UserActivityStringChanged;

	/** Sent when application code changes the currently active game session. The exact semantics of this will vary between games but it is useful for analytics, crash reports, etc  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameSessionIDChange, const FString&);
	static FOnGameSessionIDChange GameSessionIDChanged;

	/** Sent when application code changes game state. The exact semantics of this will vary between games but it is useful for analytics, crash reports, etc  */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameStateClassChange, const FString&);
	static FOnGameStateClassChange GameStateClassChanged;

	/** Sent by application code to set params that customize crash reporting behavior. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCrashOverrideParamsChanged, const FCrashOverrideParameters&);
	static FOnCrashOverrideParamsChanged CrashOverrideParamsChanged;
	
	/** Sent by engine code when the "vanilla" status of the engine changes */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnIsVanillaProductChanged, bool);
	static FOnIsVanillaProductChanged IsVanillaProductChanged;

	// Callback for platform specific very early init code.
	DECLARE_MULTICAST_DELEGATE(FOnPreMainInit);
	static FOnPreMainInit& GetPreMainInitDelegate();
	
	/** Sent when GConfig is finished initializing */
	DECLARE_MULTICAST_DELEGATE(FConfigReadyForUse);
	static FConfigReadyForUse ConfigReadyForUse;

	/** Callback for notifications regarding changes of the rendering thread. */
	DECLARE_MULTICAST_DELEGATE(FRenderingThreadChanged)

	/** Sent just after the rendering thread has been created. */
	static FRenderingThreadChanged PostRenderingThreadCreated;
	/* Sent just before the rendering thread is destroyed. */
	static FRenderingThreadChanged PreRenderingThreadDestroyed;

	// Callback to allow custom resolution of package names. Arguments are InRequestedName, OutResolvedName.
	// Should return True of resolution occured.
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FResolvePackageNameDelegate, const FString&, FString&);
	static TArray<FResolvePackageNameDelegate> PackageNameResolvers;

	// Called to request that systems free whatever memory they are able to. Called early in LoadMap.
	// Caller is responsible for flushing rendering etc. See UEngine::TrimMemory
	static FSimpleMulticastDelegate& GetMemoryTrimDelegate();

	// Called when OOM event occurs, after backup memory has been freed, so there's some hope of being effective
	static FSimpleMulticastDelegate& GetOutOfMemoryDelegate();

	enum class EOnScreenMessageSeverity : uint8
	{
		Info,
		Warning,
		Error,
	};
	typedef TMultiMap<EOnScreenMessageSeverity, FText> FSeverityMessageMap;

	// Called when displaying on screen messages (like the "Lighting needs to be rebuilt"), to let other systems add any messages as needed
	// Sample Usage:
	// void GetMyOnScreenMessages(FCoreDelegates::FSeverityMessageMap& OutMessages)
	// {
	//		OutMessages.Add(FCoreDelegates::EOnScreenMessageSeverity::Info, FText::Format(LOCTEXT("MyMessage", "My Status: {0}"), SomeStatus));
	// }
	DECLARE_MULTICAST_DELEGATE_OneParam(FGetOnScreenMessagesDelegate, FSeverityMessageMap&);
	static FGetOnScreenMessagesDelegate OnGetOnScreenMessages;

	DECLARE_DELEGATE_RetVal(bool, FIsLoadingMovieCurrentlyPlaying)
	static FIsLoadingMovieCurrentlyPlaying IsLoadingMovieCurrentlyPlaying;

	// Callback to allow user code to prevent url from being launched from FPlatformProcess::LaunchURL. Used to apply http allow list
	// Return true for to launch the url
	DECLARE_DELEGATE_RetVal_OneParam(bool, FShouldLaunchUrl, const TCHAR* /* URL */);
	static FShouldLaunchUrl ShouldLaunchUrl;

	/** Sent when GC finish destroy takes more time than expected */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGCFinishDestroyTimeExtended, const FString&);
	static FOnGCFinishDestroyTimeExtended OnGCFinishDestroyTimeExtended;

	/** Called when the application's network initializes or shutdowns on platforms where the network stack is not always available */
	DECLARE_MULTICAST_DELEGATE_OneParam(FApplicationNetworkInitializationChanged, bool /*bIsNetworkInitialized*/);
	static FApplicationNetworkInitializationChanged ApplicationNetworkInitializationChanged;

	// Callback to let code read or write specialized binary data that is generated at Stage time, for optimizing data right before 
	// final game data is being written to disk
	// The TMap is a map of an identifier for owner of the data, and a boolean where true means the data is being generated (ie editor), and false 
	// means the data is for use (ie runtime game)
	struct FExtraBinaryConfigData
	{
		// the data that will be saved/loaded quickly
		TMap<FString, TArray<uint8>> Data;

		// Ini config data (not necessarily GConfig)
		class FConfigCacheIni& Config;

		// if true, the callback should fill out Data/Config
		bool bIsGenerating;

		FExtraBinaryConfigData(class FConfigCacheIni& InConfig, bool InIsGenerating)
			: Config(InConfig)
			, bIsGenerating(InIsGenerating)
		{
		}
	};
	DECLARE_MULTICAST_DELEGATE_OneParam(FAccesExtraBinaryConfigData, FExtraBinaryConfigData&);
	static FAccesExtraBinaryConfigData AccessExtraBinaryConfigData;

	using FAttachShaderReadRequestFunc = TFunctionRef<class FIoRequest(const class FIoChunkId&, FGraphEventRef)>;
	DECLARE_DELEGATE_TwoParams(FPreloadPackageShaderMaps, TArrayView<const FSHAHash>, FAttachShaderReadRequestFunc);
	static FPreloadPackageShaderMaps PreloadPackageShaderMaps;
	DECLARE_DELEGATE_OneParam(FReleasePreloadedPackageShaderMaps, TArrayView<const FSHAHash>);
	static FReleasePreloadedPackageShaderMaps ReleasePreloadedPackageShaderMaps;
	/** Called when the verbosity of a log category is changed */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLogVerbosityChanged, const FLogCategoryName& /* CategoryName */, ELogVerbosity::Type /* OldVerbosity */, ELogVerbosity::Type /* NewVerbosity */);
	static FOnLogVerbosityChanged OnLogVerbosityChanged;

	DECLARE_DELEGATE_RetVal(TSharedPtr<class IPackageStore>, FCreatePackageStore);
	static FCreatePackageStore CreatePackageStore;

private:

	// Callbacks for hotfixes
	static TArray<FHotFixDelegate> HotFixDelegates;

	// This class is only for namespace use
	FCoreDelegates() {}
};
