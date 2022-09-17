// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_TextLayout_generated_h
#error "TextLayout.generated.h already included, missing '#pragma once' in TextLayout.h"
#endif
#define SLATE_TextLayout_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Framework_Text_TextLayout_h


#define FOREACH_ENUM_ETEXTJUSTIFY(op) \
	op(ETextJustify::Left) \
	op(ETextJustify::Center) \
	op(ETextJustify::Right) 
#define FOREACH_ENUM_ETEXTWRAPPINGPOLICY(op) \
	op(ETextWrappingPolicy::DefaultWrapping) \
	op(ETextWrappingPolicy::AllowPerCharacterWrapping) 

enum class ETextWrappingPolicy : uint8;
template<> SLATE_API UEnum* StaticEnum<ETextWrappingPolicy>();

#define FOREACH_ENUM_ETEXTFLOWDIRECTION(op) \
	op(ETextFlowDirection::Auto) \
	op(ETextFlowDirection::LeftToRight) \
	op(ETextFlowDirection::RightToLeft) 

enum class ETextFlowDirection : uint8;
template<> SLATE_API UEnum* StaticEnum<ETextFlowDirection>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
