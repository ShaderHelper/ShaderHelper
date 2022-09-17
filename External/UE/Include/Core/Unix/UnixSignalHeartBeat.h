// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreGlobals.h"
#include "CoreTypes.h"

#include <time.h>

class CORE_API FUnixSignalGameHitchHeartBeat
{
public:
	static FUnixSignalGameHitchHeartBeat* Singleton;

	/** Gets the heartbeat singleton */
	static FUnixSignalGameHitchHeartBeat& Get();
	static FUnixSignalGameHitchHeartBeat* GetNoInit();

	/**
	* Called at the start of a frame to register the time we are looking to detect a hitch
	*/
	void FrameStart(bool bSkipThisFrame = false);

	double GetFrameStartTime();
	double GetCurrentTime();

	/**
	* Suspend heartbeat hitch detection. Must call ResumeHeartBeat later to resume.
	*/
	void SuspendHeartBeat();

	/**
	* Resume heartbeat hitch detection. Call only after first calling SuspendHeartBeat.
	*/
	void ResumeHeartBeat();

	void Restart();
	void Stop();

private:
	FUnixSignalGameHitchHeartBeat();
	~FUnixSignalGameHitchHeartBeat();

	void Init();
	void InitSettings();

    double HitchThresholdS = -1.0;
	double StartTime = 0.0;
	bool bHasCmdLine = false;
	bool bDisabled = false;
	int32 SuspendCount = 0;
	timer_t TimerId = nullptr;
};
