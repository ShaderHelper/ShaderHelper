// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/IInputInterface.h"


/**
 * Interface for the force feedback system.
 *
 * Note: This class is deprecated and will be removed in favor of IInputInterface
 */
class IForceFeedbackSystem
	: public IInputInterface
{
public:

	virtual ~IForceFeedbackSystem() {};
};
