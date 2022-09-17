// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_FlowDirection_generated_h
#error "FlowDirection.generated.h already included, missing '#pragma once' in FlowDirection.h"
#endif
#define SLATECORE_FlowDirection_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Layout_FlowDirection_h


#define FOREACH_ENUM_EFLOWDIRECTIONPREFERENCE(op) \
	op(EFlowDirectionPreference::Inherit) \
	op(EFlowDirectionPreference::Culture) \
	op(EFlowDirectionPreference::LeftToRight) \
	op(EFlowDirectionPreference::RightToLeft) 

enum class EFlowDirectionPreference : uint8;
template<> SLATECORE_API UEnum* StaticEnum<EFlowDirectionPreference>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
