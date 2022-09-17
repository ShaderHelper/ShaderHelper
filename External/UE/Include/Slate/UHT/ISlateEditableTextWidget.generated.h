// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_ISlateEditableTextWidget_generated_h
#error "ISlateEditableTextWidget.generated.h already included, missing '#pragma once' in ISlateEditableTextWidget.h"
#endif
#define SLATE_ISlateEditableTextWidget_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Widgets_Text_ISlateEditableTextWidget_h


#define FOREACH_ENUM_EVIRTUALKEYBOARDTRIGGER(op) \
	op(EVirtualKeyboardTrigger::OnFocusByPointer) \
	op(EVirtualKeyboardTrigger::OnAllFocusEvents) 

enum class EVirtualKeyboardTrigger : uint8;
template<> SLATE_API UEnum* StaticEnum<EVirtualKeyboardTrigger>();

#define FOREACH_ENUM_EVIRTUALKEYBOARDDISMISSACTION(op) \
	op(EVirtualKeyboardDismissAction::TextChangeOnDismiss) \
	op(EVirtualKeyboardDismissAction::TextCommitOnAccept) \
	op(EVirtualKeyboardDismissAction::TextCommitOnDismiss) 

enum class EVirtualKeyboardDismissAction : uint8;
template<> SLATE_API UEnum* StaticEnum<EVirtualKeyboardDismissAction>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
