// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "Delegates/Delegate.h"

typedef TMap<FString, FString> FEmbeddedCommunicationMap;

// wraps parameters and a completion delegate
struct CORE_API FEmbeddedCallParamsHelper
{
	// the command for this call. something like "console" would be a command for the engine to run a console command
	FString Command;
	
	// a map of arbitrary key-value string pairs.
	FEmbeddedCommunicationMap Parameters;

	// a delegate to call back on the other end when the command is completed. if this is set, it is expected it will be called
	// but at least one handler of this
	TFunction<void(const FEmbeddedCommunicationMap&, FString)> OnCompleteDelegate;
};

class CORE_API FEmbeddedDelegates
{
public:

	// delegate for calling between native wrapper app and embedded ue
	DECLARE_MULTICAST_DELEGATE_OneParam(FEmbeddedCommunicationParamsDelegate, const FEmbeddedCallParamsHelper&);

	// calling in from native wrapper to engine
	static FEmbeddedCommunicationParamsDelegate& GetNativeToEmbeddedParamsDelegateForSubsystem(FName SubsystemName);

	// calling out from engine to native wrapper
	static FEmbeddedCommunicationParamsDelegate& GetEmbeddedToNativeParamsDelegateForSubsystem(FName SubsystemName);

	// returns true if NativeToEmbedded delegate for subsystem exists
	static bool IsEmbeddedSubsystemAvailable(FName SubsystemName);
	
	// FTSTicker-like delegate, to bind things to be ticked at a regular interval while the game thread is otherwise asleep.
	static FSimpleMulticastDelegate SleepTickDelegate;

	// get/set an object by name, thread safe
	static void SetNamedObject(const FString& Name, void* Object);
	static void* GetNamedObject(const FString& Name);
	
private:

	// This class is only for namespace use (like FCoreDelegates)
	FEmbeddedDelegates() {}

	// the per-subsystem delegates
	static TMap<FName, FEmbeddedCommunicationParamsDelegate> NativeToEmbeddedDelegateMap;
	static TMap<FName, FEmbeddedCommunicationParamsDelegate> EmbeddedToNativeDelegateMap;

	// named object registry, with thread protection
	static FCriticalSection NamedObjectRegistryLock;
	static TMap<FString, void*> NamedObjectRegistry;
};


class CORE_API FEmbeddedCommunication
{
public:

	// called early in UE lifecycle - RunOnGameThread can be called before this is called
	static void Init();

	// force some ticking to happen - used to process messages during otherwise blocking operations like boot
	static void ForceTick(int ID, float MinTimeSlice=0.1f, float MaxTimeSlice=0.5f);

	// queue up a function to call on game thread
	static void RunOnGameThread(int Priority, TFunction<void()> Lambda);

	// wake up the game thread to process something put onto the game thread
	static void WakeGameThread();

	// called from game thread to pull off
	static bool TickGameThread(float DeltaTime);
	
	// tell UE to stay awake (or allow it to sleep when nothing to do). Repeated calls with the same Requester are
	// allowed, but bNeedsRendering must match
	static void KeepAwake(FName Requester, bool bNeedsRendering);
	static void AllowSleep(FName Requester);
	
	static void UELogFatal(const TCHAR* String);
	static void UELogError(const TCHAR* String);
	static void UELogWarning(const TCHAR* String);
	static void UELogDisplay(const TCHAR* String);
	static void UELogLog(const TCHAR* String);
	static void UELogVerbose(const TCHAR* String);

	static bool IsAwakeForTicking();
	static bool IsAwakeForRendering();
	
	static FString GetDebugInfo();
};

// RAII for keep awake functionality
// This may seem a bit over engineered, but it needs to have move semantics so that any aggregate that
// includes it doesn't lose move semantics.
class FEmbeddedKeepAwake
{
public:
	// tell UE to stay awake (or allow it to sleep when nothing to do). Repeated calls with the same Requester are
	// allowed, but bNeedsRendering must match
	FEmbeddedKeepAwake(FName InRequester, bool bInNeedsRendering)
		: Requester(InRequester)
		, bNeedsRendering(bInNeedsRendering)
	{
		FEmbeddedCommunication::KeepAwake(Requester, bNeedsRendering);
	}

	FEmbeddedKeepAwake(const FEmbeddedKeepAwake& Other)
		: Requester(Other.Requester)
		, bNeedsRendering(Other.bNeedsRendering)
		, bIsValid(Other.bIsValid)
	{
		if (bIsValid)
		{
			FEmbeddedCommunication::KeepAwake(Requester, bNeedsRendering);
		}
	}

	FEmbeddedKeepAwake(FEmbeddedKeepAwake&& Other)
	{
		Requester = Other.Requester;
		bNeedsRendering = Other.bNeedsRendering;
		bIsValid = Other.bIsValid;

		Other.Requester = NAME_None;
		Other.bNeedsRendering = false;
		Other.bIsValid = false;
	}

	~FEmbeddedKeepAwake()
	{
		if (bIsValid)
		{
			FEmbeddedCommunication::AllowSleep(Requester);
		}
	}

	FEmbeddedKeepAwake& operator=(const FEmbeddedKeepAwake& Other) &
	{
		bool bOldIsValid = bIsValid;
		FName OldRequester = Requester;

		Requester = Other.Requester;
		bNeedsRendering = Other.bNeedsRendering;
		bIsValid = Other.bIsValid;
		if (bIsValid)
		{
			FEmbeddedCommunication::KeepAwake(Requester, bNeedsRendering);
		}

		if (bOldIsValid)
		{
			FEmbeddedCommunication::AllowSleep(OldRequester);
		}

		return *this;
	}

	FEmbeddedKeepAwake& operator=(FEmbeddedKeepAwake&& Other) &
	{
		if (bIsValid)
		{
			FEmbeddedCommunication::AllowSleep(Requester);
		}

		Requester = Other.Requester;
		bNeedsRendering = Other.bNeedsRendering;
		bIsValid = Other.bIsValid;

		Other.Requester = NAME_None;
		Other.bNeedsRendering = false;
		Other.bIsValid = false;
		return *this;
	}

	bool GetNeedsRendering() const { return bNeedsRendering; }
	FName GetRequester() const { return Requester;  }

private:
	FName Requester;
	bool bNeedsRendering = false;
	bool bIsValid = true;
};

