// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	HoloLensIncludes.h: Includes the platform specific headers for HoloLens
==================================================================================*/

#pragma once

// Set up compiler pragmas, etc
#include "HoloLens/HoloLensCompilerSetup.h"

// include platform implementations
#include "HoloLens/HoloLensMemory.h"
#include "HoloLens/HoloLensString.h"
#include "HoloLens/HoloLensMisc.h"
#include "HoloLens/HoloLensStackWalk.h"
#include "HoloLens/HoloLensMath.h"
#include "HoloLens/HoloLensTime.h"
#include "HoloLens/HoloLensProcess.h"
#include "HoloLens/HoloLensOutputDevices.h"
#include "HoloLens/HoloLensAtomics.h"
#include "HoloLens/HoloLensTLS.h"
#include "HoloLens/HoloLensSplash.h"
#include "HoloLens/HoloLensSurvey.h"

typedef FGenericPlatformRHIFramePacer FPlatformRHIFramePacer;

typedef FGenericPlatformAffinity FPlatformAffinity;

#include "HoloLensProperties.h"
