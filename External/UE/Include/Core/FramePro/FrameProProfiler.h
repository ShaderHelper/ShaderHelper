// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FramePro/FramePro.h"

#if FRAMEPRO_ENABLED

/** Wrapper for FramePro  */
class CORE_API FFrameProProfiler
{
public:
	/** Called to mark the start of each frame  */
	static void FrameStart();

	/** Begin a named event */
	static void PushEvent(); // Event with no name, expected to be named at the end
	static void PushEvent(const TCHAR* Text);
	static void PushEvent(const ANSICHAR* Text);

	/** End currently active named event */
	static void PopEvent();
	static void PopEvent(const TCHAR* Override);
	static void PopEvent(const ANSICHAR* Override);

	static void StartFrameProRecordingFromCommand(const TArray< FString >& Args);
	static FString StartFrameProRecording(const FString& FilenameRoot, int32 MinScopeTime);
	static void StopFrameProRecording();

	static bool IsFrameProRecording();
};

#endif // FRAMEPRO_ENABLED