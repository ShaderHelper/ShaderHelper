// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_UICommandInfo_generated_h
#error "UICommandInfo.generated.h already included, missing '#pragma once' in UICommandInfo.h"
#endif
#define SLATE_UICommandInfo_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Framework_Commands_UICommandInfo_h


#define FOREACH_ENUM_EUSERINTERFACEACTIONTYPE(op) \
	op(EUserInterfaceActionType::None) \
	op(EUserInterfaceActionType::Button) \
	op(EUserInterfaceActionType::ToggleButton) \
	op(EUserInterfaceActionType::RadioButton) \
	op(EUserInterfaceActionType::Check) \
	op(EUserInterfaceActionType::CollapsedButton) 

enum class EUserInterfaceActionType : uint8;
template<> SLATE_API UEnum* StaticEnum<EUserInterfaceActionType>();

#define FOREACH_ENUM_EMULTIPLEKEYBINDINGINDEX(op) \
	op(EMultipleKeyBindingIndex::Primary) \
	op(EMultipleKeyBindingIndex::Secondary) \
	op(EMultipleKeyBindingIndex::NumChords) 

enum class EMultipleKeyBindingIndex : uint8;
template<> SLATE_API UEnum* StaticEnum<EMultipleKeyBindingIndex>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
