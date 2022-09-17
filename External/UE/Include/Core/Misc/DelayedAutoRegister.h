// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/Function.h"

enum class EDelayedRegisterRunPhase : uint8
{
	StartOfEnginePreInit,
	FileSystemReady,
	StatSystemReady,
	IniSystemReady,
	TaskGraphSystemReady,
	EarliestPossiblePluginsLoaded,
	ShaderTypesReady,
	PreObjectSystemReady,
	ObjectSystemReady,
	EndOfEngineInit,

	NumPhases,
};

struct CORE_API FDelayedAutoRegisterHelper
{

	FDelayedAutoRegisterHelper(EDelayedRegisterRunPhase RunPhase, TFunction<void()> RegistrationFunction);

	static void RunAndClearDelayedAutoRegisterDelegates(EDelayedRegisterRunPhase RunPhase);
};
