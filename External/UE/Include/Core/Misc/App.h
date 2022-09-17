// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Parse.h"
#include "UObject/NameTypes.h"
#include "CoreGlobals.h"
#include "Delegates/Delegate.h"
#include "Misc/Guid.h"
#include "Misc/CoreMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Optional.h"
#include "Misc/QualifiedFrameTime.h"
#include "HAL/PlatformProcess.h"

/**
 * Provides information about the application.
 */
class CORE_API FApp
{
public:

	/**
	 * Gets the name of the version control branch that this application was built from.
	 *
	 * @return The branch name.
	 */
	static FString GetBranchName();

	/**
	 * Gets the application's build configuration, i.e. Debug or Shipping.
	 *
	 * @return The build configuration.
	 */
	static EBuildConfiguration GetBuildConfiguration();

	/**
	 * Gets the target type of the current application (eg. client, server, etc...)
	 *
	 * @return The build target type
	 */
	static EBuildTargetType GetBuildTargetType();

#if UE_BUILD_DEVELOPMENT
	/**
	 * For development configurations, sets whether the application should load DebugGame game modules.
	 *
	 * @param Whether we're running in debug game or not.
	 */
	static void SetDebugGame(bool bIsDebugGame);
#endif

	/*
	* Gets the unique version string for this build. This string is not assumed to have any particular format other being a unique identifier for the build.
	*
	* @return The build version
	*/
	static const TCHAR* GetBuildVersion();

	/**
	 * Gets the date at which this application was built.
	 *
	 * @return Build date string.
	 */
	static FString GetBuildDate();

	/**
	 * Gets the name of the graphics RHI currently in use.
	 *
	 * @return name of Graphics RHI
	 */
	static FString GetGraphicsRHI();

	/**
	 * Sets the Graphics RHI currently in use
	 */
	static void SetGraphicsRHI(FString RHIString);


	/**
	 * Gets the value of ENGINE_IS_PROMOTED_BUILD.
	 */
	static int32 GetEngineIsPromotedBuild();

	/**
	 * Gets the identifier for the unreal engine
	 */
	static FString GetEpicProductIdentifier();

	/**
	 * Gets the name of the current project.
	 *
	 * @return The project name.
	 */
	FORCEINLINE static const TCHAR* GetProjectName()
	{
		return GInternalProjectName;
	}

	/**
	 * Gets the name of the application, i.e. "UE" or "Rocket".
	 *
	 * @todo need better application name discovery. this is quite horrible and may not work on future platforms.
	 * @return Application name string.
	 */
	static FString GetName()
	{
		FString ExecutableName = FPlatformProcess::ExecutableName();

		int32 ChopIndex = ExecutableName.Find(TEXT("-"));

		if (ExecutableName.FindChar(TCHAR('-'), ChopIndex))
		{
			return ExecutableName.Left(ChopIndex);
		}

		if (ExecutableName.FindChar(TCHAR('.'), ChopIndex))
		{
			return ExecutableName.Left(ChopIndex);
		}

		return ExecutableName;
	}

	/**
	 * Reports if the project name has been set
	 *
	 * @return true if the project name has been set
	 */
	FORCEINLINE static bool HasProjectName()
	{
		return (IsProjectNameEmpty() == false) && (FCString::Stricmp(GInternalProjectName, TEXT("None")) != 0);
	}

	/**
	 * Checks whether this application is a game.
	 *
	 * Returns true if a normal or PIE game is active (basically !GIsEdit or || GIsPlayInEditorWorld)
	 * This must NOT be accessed on threads other than the game thread!
	 * Use View->Family->EnginShowFlags.Game on the rendering thread.
	 *
	 * @return true if the application is a game, false otherwise.
	 */
	FORCEINLINE static bool IsGame()
	{
#if WITH_EDITOR
		return !GIsEditor || GIsPlayInEditorWorld || IsRunningGame();
#else
		return true;
#endif
	}

	/**
	 * Reports if the project name is empty
	 *
	 * @return true if the project name is empty
	 */
	FORCEINLINE static bool IsProjectNameEmpty()
	{
		return (GInternalProjectName[0] == 0);
	}

	/**
	* Sets the name of the current project.
	*
	* @param InProjectName Name of the current project.
	*/
	FORCEINLINE static void SetProjectName(const TCHAR* InProjectName)
	{
		// At the moment Strcpy is not safe as we don't check the buffer size on all platforms, so we use strncpy here.
		FCString::Strncpy(GInternalProjectName, InProjectName, UE_ARRAY_COUNT(GInternalProjectName));
		// And make sure the ProjectName string is null terminated.
		GInternalProjectName[UE_ARRAY_COUNT(GInternalProjectName) - 1] = TEXT('\0');
	}

public:

	/**
	 * Add the specified user to the list of authorized session users.
	 *
	 * @param UserName The name of the user to add.
	 * @see DenyUser, DenyAllUsers, IsAuthorizedUser
	 */
	FORCEINLINE static void AuthorizeUser(const FString& UserName)
	{
		SessionUsers.AddUnique(UserName);
	}

	/**
	 * Removes all authorized users.
	 *
	 * @see AuthorizeUser, DenyUser, IsAuthorizedUser
	 */
	FORCEINLINE static void DenyAllUsers()
	{
		SessionUsers.Empty();
	}

	/**
	 * Remove the specified user from the list of authorized session users.
	 *
	 * @param UserName The name of the user to remove.
	 * @see AuthorizeUser, DenyAllUsers, IsAuthorizedUser
	 */
	FORCEINLINE static void DenyUser(const FString& UserName)
	{
		SessionUsers.Remove(UserName);
	}

	/**
	 * Gets the globally unique identifier of this application instance.
	 *
	 * Every running instance of the engine has a globally unique instance identifier
	 * that can be used to identify it from anywhere on the network.
	 *
	 * @return Instance identifier, or an invalid GUID if there is no local instance.
	 * @see GetSessionId
	 */
	FORCEINLINE static FGuid GetInstanceId()
	{
		static FGuid InstanceId = FGuid::NewGuid();
		return InstanceId;
	}

	/**
	 * Gets the name of this application instance.
	 *
	 * By default, the instance name is a combination of the computer name and process ID.
	 *
	 * @return Instance name string.
	 */
	static FString GetInstanceName()
	{
		return FString::Printf(TEXT("%s-%i"), FPlatformProcess::ComputerName(), FPlatformProcess::GetCurrentProcessId());
	}

	/**
	 * Gets the identifier of the session that this application is part of.
	 *
	 * A session is group of applications that were launched and logically belong together.
	 * For example, when starting a new session in UFE that launches a game on multiple devices,
	 * all engine instances running on those devices will have the same session identifier.
	 * Conversely, sessions that were launched separately will have different session identifiers.
	 *
	 * @return Session identifier, or an invalid GUID if there is no local instance.
	 * @see GetInstanceId
	 */
	FORCEINLINE static FGuid GetSessionId()
	{
		return SessionId;
	}

	/**
	 * Gets the name of the session that this application is part of, if any.
	 *
	 * @return Session name string.
	 */
	FORCEINLINE static FString GetSessionName()
	{
		return SessionName;
	}

	/**
	 * Gets the name of the user who owns the session that this application is part of, if any.
	 *
	 * If this application is part of a session that was launched from UFE, this function
	 * will return the name of the user that launched the session. If this application is
	 * not part of a session, this function will return the name of the local user account
	 * under which the application is running.
	 *
	 * @return Name of session owner.
	 */
	FORCEINLINE static FString GetSessionOwner()
	{
		return SessionOwner;
	}

	/**
	 * Initializes the application session.
	 */
	static void InitializeSession();

	/**
	 * Check whether the specified user is authorized to interact with this session.
	 *
	 * @param UserName The name of the user to check.
	 * @return true if the user is authorized, false otherwise.
	 * @see AuthorizeUser, DenyUser, DenyAllUsers
	 */
	FORCEINLINE static bool IsAuthorizedUser(const FString& UserName)
	{
		return ((FPlatformProcess::UserName(false) == UserName) || (SessionOwner == UserName) || SessionUsers.Contains(UserName));
	}

	/**
	 * Checks whether this is a standalone application.
	 *
	 * An application is standalone if it has not been launched as part of a session from UFE.
	 * If an application was launched from UFE, it is part of a session and not considered standalone.
	 *
	 * @return true if this application is standalone, false otherwise.
	 */
	FORCEINLINE static bool IsStandalone()
	{
		return Standalone;
	}

	/**
	 * Check whether the given instance ID identifies this instance.
	 *
	 * @param InInstanceId The instance ID to check.
	 * @return true if the ID identifies this instance, false otherwise.
	 */
	FORCEINLINE static bool IsThisInstance(const FGuid& InInstanceId)
	{
		return (InInstanceId == GetInstanceId());
	};

	/**
	 * Set a new session name.
	 *
	 * @param NewName The new session name.
	 * @see SetSessionOwner
	 */
	FORCEINLINE static void SetSessionName(const FString& NewName)
	{
		SessionName = NewName;
	}

	/**
	 * Set a new session owner.
	 *
	 * @param NewOwner The name of the new owner.
	 * @see SetSessionName
	 */
	FORCEINLINE static void SetSessionOwner(const FString& NewOwner)
	{
		SessionOwner = NewOwner;
	}

public:

// MSVC 16.4 (1924) has a bug that does not properly handle the local static bool in CanEverRender. This will be fixed in 16.5. Working around by using FORCENOINLINE.
// MSVC 16.6 (1926) fixes the problem so inlning can be enabled again from that point onwards.
#if !UE_SERVER && defined(_MSC_VER) && _MSC_VER >= 1924 && _MSC_VER <= 1925
#define INLINE_CANEVERRENDER FORCENOINLINE
#else
#define INLINE_CANEVERRENDER FORCEINLINE
#endif

	/**
	 * Checks whether this application can render anything.
	 * Certain application types never render, while for others this behavior may be controlled by switching to NullRHI.
	 * This can be used for decisions like omitting code paths that make no sense on servers or games running in headless mode (e.g. automated tests).
	 *
	 * @return true if the application can render, false otherwise.
	 */
	INLINE_CANEVERRENDER static bool CanEverRender()
	{
#if UE_SERVER
		return false;
#else
		static bool bHasNullRHIOnCommandline = FParse::Param(FCommandLine::Get(), TEXT("nullrhi"));
		return (!IsRunningCommandlet() || IsAllowCommandletRendering()) && !IsRunningDedicatedServer() && !(USE_NULL_RHI || bHasNullRHIOnCommandline);
#endif // UE_SERVER
	}

	/**
	 * Checks whether this application can render audio.
	 * Certain application types produce sound, while for others this can be controlled via the -nosound cmdline.
	 * This can be used for decisions like omitting code paths that make no sense on servers or games running in headless mode (e.g. automated tests).
	 *
	 * @return true if the application can render audio, false otherwise.
	 */
	INLINE_CANEVERRENDER static bool CanEverRenderAudio()
	{
#if UE_SERVER
		return false;
#else
		static bool bHasNoAudioOnCommandline = FParse::Param(FCommandLine::Get(), TEXT("nosound")) && !FParse::Param(FCommandLine::Get(), TEXT("enablesound"));
		static bool bApplicationTypeDoesNotRenderAudio = FApp::IsBenchmarking() || IsRunningDedicatedServer() || (IsRunningCommandlet() && !IsAllowCommandletAudio());
		return !bApplicationTypeDoesNotRenderAudio && !bHasNoAudioOnCommandline;
#endif // UE_SERVER
	}

	/**
	 * Checks whether this application has been installed.
	 *
	 * Non-server desktop shipping builds are assumed to be installed.
	 *
	 * Installed applications may not write to the folder in which they exist since they are likely in a system folder (like "Program Files" in windows).
	 * Instead, they should write to the OS-specific user folder FPlatformProcess::UserDir() or application data folder FPlatformProcess::ApplicationSettingsDir()
	 * User Access Control info for windows Vista and newer: http://en.wikipedia.org/wiki/User_Account_Control
	 *
	 * To run an "installed" build without installing, use the -Installed command line parameter.
	 * To run a "non-installed" desktop shipping build, use the -NotInstalled command line parameter.
	 *
	 * @return true if the application is installed, false otherwise.
	 */
	static bool IsInstalled();

	/**
	 * Checks whether the engine components of this application have been installed.
	 *
	 * In binary Unreal Engine releases, the engine can be installed while the game is not. The game IsInstalled()
	 * setting will take precedence over this flag.
	 *
	 * To override, pass -engineinstalled or -enginenotinstalled on the command line.
	 *
	 * @return true if the engine is installed, false otherwise.
	 */
	static bool IsEngineInstalled();

	/**
	 * Checks whether this application runs unattended.
	 *
	 * Unattended applications are not monitored by anyone or are unable to receive user input.
	 * This method can be used to determine whether UI pop-ups or other dialogs should be shown.
	 *
	 * @return true if the application runs unattended, false otherwise.
	 */
	static bool IsUnattended();

	/**
	 * Checks whether the application should run multi-threaded for performance critical features.
	 *
	 * This method is used for performance based threads (like rendering, task graph).
	 * This will not disable async IO or similar threads needed to disable hitching
	 *
	 * @return true if this isn't a server, has more than one core, does not have a -onethread command line options, etc.
	 */
	static bool ShouldUseThreadingForPerformance();

	/**
	 * Checks whether application is in benchmark mode.
	 *
	 * @return true if application is in benchmark mode, false otherwise.
	 */
	FORCEINLINE static bool IsBenchmarking()
	{
		return bIsBenchmarking;
	}

	/**
	 * Sets application benchmarking mode.
	 *
	 * @param bVal True sets application in benchmark mode, false sets to non-benchmark mode.
	 */
	static void SetBenchmarking(bool bVal)
	{
		bIsBenchmarking = bVal;
	}

	/**
	 * Gets time step in seconds if a fixed delta time is wanted.
	 *
	 * @return Time step in seconds for fixed delta time.
	 */
	FORCEINLINE static double GetFixedDeltaTime()
	{
		return FixedDeltaTime;
	}

	/**
	 * Sets time step in seconds if a fixed delta time is wanted.
	 *
	 * @param seconds Time step in seconds for fixed delta time.
	 */
	static void SetFixedDeltaTime(double Seconds)
	{
		FixedDeltaTime = Seconds;
	}

	/**
	 * Gets whether we want to use a fixed time step or not.
	 *
	 * @return True if using fixed time step, false otherwise.
	 */
	static bool UseFixedTimeStep()
	{
		return bUseFixedTimeStep;
	}

	/**
	 * Enables or disabled usage of fixed time step.
	 *
	 * @param whether to use fixed time step or not
	 */
	static void SetUseFixedTimeStep(bool bVal)
	{
		bUseFixedTimeStep = bVal;
	}

	/**
	 * Gets current time in seconds.
	 *
	 * @return Current time in seconds.
	 */
	FORCEINLINE static double GetCurrentTime()
	{
		return CurrentTime;
	}

	/**
	 * Sets current time in seconds.
	 *
	 * @param seconds - Time in seconds.
	 */
	static void SetCurrentTime(double Seconds)
	{
		CurrentTime = Seconds;
	}

	/**
	 * Gets previous value of CurrentTime.
	 *
	 * @return Previous value of CurrentTime.
	 */
	FORCEINLINE static double GetLastTime()
	{
		return LastTime;
	}

	/** Updates Last time to CurrentTime. */
	static void UpdateLastTime()
	{
		LastTime = CurrentTime;
	}

	/**
	 * Gets time delta in seconds.
	 *
	 * @return Time delta in seconds.
	 */
	FORCEINLINE static double GetDeltaTime()
	{
		return DeltaTime;
	}

	/**
	 * Sets time delta in seconds.
	 *
	 * @param seconds Time in seconds.
	 */
	static void SetDeltaTime(double Seconds)
	{
		DeltaTime = Seconds;
	}

	/**
	 * Gets idle time in seconds.
	 *
	 * @return Idle time in seconds.
	 */
	FORCEINLINE static double GetIdleTime()
	{
		return IdleTime;
	}

	/**
	 * Sets idle time in seconds.
	 *
	 * @param seconds - Idle time in seconds.
	 */
	static void SetIdleTime(double Seconds)
	{
		IdleTime = Seconds;
	}

	/**
	 * Gets overall game time in seconds.
	 *
	 * @return Overall game time in seconds.
	 */
	FORCEINLINE static double GetGameTime()
	{
		return GameTime;
	}

	/**
	 * Sets overall game time in seconds.
	 *
	* @param seconds - Overall game time in seconds.
	 */
	static void SetGameTime(double Seconds)
	{
		GameTime = Seconds;
	}

	/**
	 * Gets idle time overshoot in seconds (the time beyond the wait time we requested for the frame). Only valid when IdleTime is > 0.
	 *
	 * @return Idle time in seconds.
	 */
	FORCEINLINE static double GetIdleTimeOvershoot()
	{
		return IdleTimeOvershoot;
	}

	/**
	 * Sets idle time overshoot in seconds (the time beyond the wait time we requested for the frame). Only valid when IdleTime is > 0.
	 *
	 * @param seconds - Idle time in seconds.
	 */
	static void SetIdleTimeOvershoot(double Seconds)
	{
		IdleTimeOvershoot = Seconds;
	}

	/**
	 * Convert the current frame time into a readable timecode.
	 * If the current frame time is not set, the timecode value will be defaulted.
	 *
	 * @return the current timecode.
	 */
	static FTimecode GetTimecode();

	/**
	 * Get the frame rate of the current frame time.
	 * If the current frame time is not set, the frame rate value will be defaulted.
	 *
	 * @return the current timecode frame rate.
	 */
	static FFrameRate GetTimecodeFrameRate();

	/**
	 * Gets a frame number generated by the engine's timecode provider.
	 * The current frame time is used to generate a timecode value.
	 * The optional will be false if no timecode provider was set or is not in a synchronized state.
	 *
	 * @return the current frame time.
	 */
	FORCEINLINE static TOptional<FQualifiedFrameTime> GetCurrentFrameTime()
	{
		return CurrentFrameTime;
	}

	/**
	 * Sets the current timecode, and the frame rate to which it's relative.
	 *
	 * @param InTimecode - current timecode.
	 * @param InTimecodeFrameRate - current timecode framerate.
	 */
	UE_DEPRECATED(4.25, "Please use SetQualifiedFrameTime")
	static void SetTimecodeAndFrameRate(FTimecode InTimecode, FFrameRate InTimecodeFrameRate)
	{
		CurrentFrameTime = FQualifiedFrameTime(InTimecode, InTimecodeFrameRate);
	}

	/**
	 * Sets the current frame time.
	 *
	 * @param InFrameTime - current frame time and frame rate.
	 */
	static void SetCurrentFrameTime(FQualifiedFrameTime InFrameTime)
	{
		CurrentFrameTime = InFrameTime;
	}

	/** Invalidate the current frame time. It will reset the TOptional. */
	static void InvalidateCurrentFrameTime()
	{
		CurrentFrameTime.Reset();
	}

	/**
	 * Get Volume Multiplier
	 * 
	 * @return Current volume multiplier
	 */
	FORCEINLINE static float GetVolumeMultiplier()
	{
		return VolumeMultiplier;
	}

	/**
	 * Set Volume Multiplier
	 * 
	 * @param  InVolumeMultiplier	new volume multiplier
	 */
	FORCEINLINE static void SetVolumeMultiplier(float InVolumeMultiplier)
	{
		VolumeMultiplier = InVolumeMultiplier;
	}

	/**
	 * Helper function to get UnfocusedVolumeMultiplier from config and store so it's not retrieved every frame
	 * 
	 * @return Volume multiplier to use when app loses focus
	 */
	static float GetUnfocusedVolumeMultiplier();

	/**
	* Sets the Unfocused Volume Multiplier
	*/
	static void SetUnfocusedVolumeMultiplier(float InVolumeMultiplier);

	/**
	 * Sets if VRFocus should be used.
	 *
	 * @param  bInUseVRFocus	new bUseVRFocus value
	 */
	static void SetUseVRFocus(bool bInUseVRFocus);

	/**
	 * Gets if VRFocus should be used
	 */
	FORCEINLINE static bool UseVRFocus()
	{
		return bUseVRFocus;
	}
	/**
	 * Sets VRFocus, which indicates that the application should continue to render 
	 * Audio and Video as if it had window focus, even though it may not.
	 *
	 * @param  bInVRFocus	new VRFocus value
	 */
	static void SetHasVRFocus(bool bInHasVRFocus);

	/**
	 * Gets VRFocus, which indicates that the application should continue to render 
	 * Audio and Video as if it had window focus, even though it may not.
	 */
	FORCEINLINE static bool HasVRFocus()
	{
		return bHasVRFocus;
	}
	
	/**
	 * Sets HasFocus, which is a function that indicates that the application window has focus.
	 *
	 * @param  InHasFocusFunction	address of a function that can query the focus state, typically &FPlatformApplicationMisc::IsThisApplicationForeground
	 */
	static void SetHasFocusFunction(bool (*InHasFocusFunction)());

	/**
	 * Gets Focus, Indicates that the application should continue to render
	 * Audio and Video as if it had window focus, even though it may not.
	 */
	static bool HasFocus();

	/* If the random seed started with a constant or on time, can be affected by -FIXEDSEED or -BENCHMARK */
	static bool bUseFixedSeed;

	/* Print all initial startup logging */
	static void PrintStartupLogMessages();

private:

#if UE_BUILD_DEVELOPMENT
	/** The current build configuration */
	static bool bIsDebugGame;
#endif

	/** Holds the session identifier. */
	static FGuid SessionId;

	/** Holds the session name. */
	static FString SessionName;

	/** Holds the name of the user that launched session. */
	static FString SessionOwner;

	/** Holds the name the graphics RHI currently in use*/
	static FString GraphicsRHI;

	/** List of authorized session users. */
	static TArray<FString> SessionUsers;

	/** Holds a flag indicating whether this is a standalone session. */
	static bool Standalone;

	/** Holds a flag Whether we are in benchmark mode or not. */
	static bool bIsBenchmarking;

	/** Holds a flag whether we want to use a fixed time step or not. */
	static bool bUseFixedTimeStep;

	/** Holds time step if a fixed delta time is wanted. */
	static double FixedDeltaTime;

	/** Holds current time. */
	static double CurrentTime;

	/** Holds previous value of CurrentTime. */
	static double LastTime;

	/** Holds current delta time in seconds. */
	static double DeltaTime;

	/** Holds time we spent sleeping in UpdateTimeAndHandleMaxTickRate() if our frame time was smaller than one allowed by target FPS. */
	static double IdleTime;

	/** Holds the amount of IdleTime that was LONGER than we tried to sleep. The OS can't sleep the exact amount of time, so this measures that overshoot. */
	static double IdleTimeOvershoot;

	/** Holds overall game time. */
	static double GameTime;

	/** Holds the current frame time and framerate. */
	static TOptional<FQualifiedFrameTime> CurrentFrameTime;

	/** Holds if we should generate a drop frame timecode when the frame rate does support it. */
	static bool bUseDropFrameFormatWhenSupported;

	/** Use to affect the app volume when it loses focus */
	static float VolumeMultiplier;

	/** Read from config to define the volume when app loses focus */
	static float UnfocusedVolumeMultiplier;

	/** Holds a flag indicating if VRFocus should be used */
	static bool bUseVRFocus;

	/** Holds a flag indicating if app has focus in side the VR headset */
	static bool bHasVRFocus;

	/** Holds a function address that can indicate if application has focus */
	static bool (*HasFocusFunction)();
};


/** Called to determine the result of IsServerForOnlineSubsystems() */
DECLARE_DELEGATE_RetVal_OneParam(bool, FQueryIsRunningServer, FName);


/**
 * @return true if there is a running game world that is a server (including listen servers), false otherwise
 */
CORE_API bool IsServerForOnlineSubsystems(FName WorldContextHandle);

/**
 * Sets the delegate used for IsServerForOnlineSubsystems
 */
CORE_API void SetIsServerForOnlineSubsystemsDelegate(FQueryIsRunningServer NewDelegate);
