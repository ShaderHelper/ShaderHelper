// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Mac/MacErrorOutputDevice.h"

class FMacApplicationErrorOutputDevice : public FMacErrorOutputDevice
{
protected:
	virtual void HandleErrorRestoreUI() override;
};

