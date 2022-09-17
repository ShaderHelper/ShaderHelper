// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Windows/WindowsErrorOutputDevice.h"

/**
 * Windows implementation of console log window, utilizing the Win32 console API
 */
class FWindowsApplicationErrorOutputDevice : public FWindowsErrorOutputDevice
{
protected:
	virtual void HandleErrorRestoreUI() override;
};

