// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_MultiBoxDefs_generated_h
#error "MultiBoxDefs.generated.h already included, missing '#pragma once' in MultiBoxDefs.h"
#endif
#define SLATE_MultiBoxDefs_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_MultiBoxDefs_h


#define FOREACH_ENUM_EMULTIBOXTYPE(op) \
	op(EMultiBoxType::MenuBar) \
	op(EMultiBoxType::ToolBar) \
	op(EMultiBoxType::VerticalToolBar) \
	op(EMultiBoxType::SlimHorizontalToolBar) \
	op(EMultiBoxType::UniformToolBar) \
	op(EMultiBoxType::Menu) \
	op(EMultiBoxType::ButtonRow) 

enum class EMultiBoxType : uint8;
template<> SLATE_API UEnum* StaticEnum<EMultiBoxType>();

#define FOREACH_ENUM_EMULTIBLOCKTYPE(op) \
	op(EMultiBlockType::None) \
	op(EMultiBlockType::ButtonRow) \
	op(EMultiBlockType::EditableText) \
	op(EMultiBlockType::Heading) \
	op(EMultiBlockType::MenuEntry) \
	op(EMultiBlockType::Separator) \
	op(EMultiBlockType::ToolBarButton) \
	op(EMultiBlockType::ToolBarComboButton) \
	op(EMultiBlockType::Widget) 

enum class EMultiBlockType : uint8;
template<> SLATE_API UEnum* StaticEnum<EMultiBlockType>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
