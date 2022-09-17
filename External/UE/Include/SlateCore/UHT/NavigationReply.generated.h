// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_NavigationReply_generated_h
#error "NavigationReply.generated.h already included, missing '#pragma once' in NavigationReply.h"
#endif
#define SLATECORE_NavigationReply_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Input_NavigationReply_h


#define FOREACH_ENUM_EUINAVIGATIONRULE(op) \
	op(EUINavigationRule::Escape) \
	op(EUINavigationRule::Explicit) \
	op(EUINavigationRule::Wrap) \
	op(EUINavigationRule::Stop) \
	op(EUINavigationRule::Custom) \
	op(EUINavigationRule::CustomBoundary) \
	op(EUINavigationRule::Invalid) 

enum class EUINavigationRule : uint8;
template<> SLATECORE_API UEnum* StaticEnum<EUINavigationRule>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
