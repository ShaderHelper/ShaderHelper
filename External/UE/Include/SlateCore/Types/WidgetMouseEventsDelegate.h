// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Layout/Geometry.h"

/** Delegate type for handling mouse events */
DECLARE_DELEGATE_RetVal_TwoParams(
	FReply, FPointerEventHandler,
	/** The geometry of the widget*/
	const FGeometry&,
	/** The Mouse Event that we are processing */
	const FPointerEvent&)

DECLARE_DELEGATE_TwoParams(
	FNoReplyPointerEventHandler,
	/** The geometry of the widget*/
	const FGeometry&,
	/** The Mouse Event that we are processing */
	const FPointerEvent&)

DECLARE_DELEGATE_OneParam(
	FSimpleNoReplyPointerEventHandler,
	/** The Mouse Event that we are processing */
	const FPointerEvent&)
