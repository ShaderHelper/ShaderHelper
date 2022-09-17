// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_SlateDebugging_generated_h
#error "SlateDebugging.generated.h already included, missing '#pragma once' in SlateDebugging.h"
#endif
#define SLATECORE_SlateDebugging_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Debugging_SlateDebugging_h


#define FOREACH_ENUM_ESLATEDEBUGGINGINPUTEVENT(op) \
	op(ESlateDebuggingInputEvent::MouseMove) \
	op(ESlateDebuggingInputEvent::MouseEnter) \
	op(ESlateDebuggingInputEvent::MouseLeave) \
	op(ESlateDebuggingInputEvent::PreviewMouseButtonDown) \
	op(ESlateDebuggingInputEvent::MouseButtonDown) \
	op(ESlateDebuggingInputEvent::MouseButtonUp) \
	op(ESlateDebuggingInputEvent::MouseButtonDoubleClick) \
	op(ESlateDebuggingInputEvent::MouseWheel) \
	op(ESlateDebuggingInputEvent::TouchStart) \
	op(ESlateDebuggingInputEvent::TouchEnd) \
	op(ESlateDebuggingInputEvent::TouchForceChanged) \
	op(ESlateDebuggingInputEvent::TouchFirstMove) \
	op(ESlateDebuggingInputEvent::TouchMoved) \
	op(ESlateDebuggingInputEvent::DragDetected) \
	op(ESlateDebuggingInputEvent::DragEnter) \
	op(ESlateDebuggingInputEvent::DragLeave) \
	op(ESlateDebuggingInputEvent::DragOver) \
	op(ESlateDebuggingInputEvent::DragDrop) \
	op(ESlateDebuggingInputEvent::DropMessage) \
	op(ESlateDebuggingInputEvent::PreviewKeyDown) \
	op(ESlateDebuggingInputEvent::KeyDown) \
	op(ESlateDebuggingInputEvent::KeyUp) \
	op(ESlateDebuggingInputEvent::KeyChar) \
	op(ESlateDebuggingInputEvent::AnalogInput) \
	op(ESlateDebuggingInputEvent::TouchGesture) \
	op(ESlateDebuggingInputEvent::MotionDetected) 

enum class ESlateDebuggingInputEvent : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ESlateDebuggingInputEvent>();

#define FOREACH_ENUM_ESLATEDEBUGGINGSTATECHANGEEVENT(op) \
	op(ESlateDebuggingStateChangeEvent::MouseCaptureGained) \
	op(ESlateDebuggingStateChangeEvent::MouseCaptureLost) 

enum class ESlateDebuggingStateChangeEvent : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ESlateDebuggingStateChangeEvent>();

#define FOREACH_ENUM_ESLATEDEBUGGINGNAVIGATIONMETHOD(op) \
	op(ESlateDebuggingNavigationMethod::Unknown) \
	op(ESlateDebuggingNavigationMethod::Explicit) \
	op(ESlateDebuggingNavigationMethod::CustomDelegateBound) \
	op(ESlateDebuggingNavigationMethod::CustomDelegateUnbound) \
	op(ESlateDebuggingNavigationMethod::NextOrPrevious) \
	op(ESlateDebuggingNavigationMethod::HitTestGrid) 

enum class ESlateDebuggingNavigationMethod : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ESlateDebuggingNavigationMethod>();

#define FOREACH_ENUM_ESLATEDEBUGGINGFOCUSEVENT(op) \
	op(ESlateDebuggingFocusEvent::FocusChanging) \
	op(ESlateDebuggingFocusEvent::FocusLost) \
	op(ESlateDebuggingFocusEvent::FocusReceived) 

enum class ESlateDebuggingFocusEvent : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ESlateDebuggingFocusEvent>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
