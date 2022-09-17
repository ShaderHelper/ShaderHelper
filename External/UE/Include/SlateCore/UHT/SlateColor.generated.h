// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_SlateColor_generated_h
#error "SlateColor.generated.h already included, missing '#pragma once' in SlateColor.h"
#endif
#define SLATECORE_SlateColor_generated_h

#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_SlateColor_h_43_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FSlateColor_Statics; \
	static class UScriptStruct* StaticStruct();


template<> SLATECORE_API UScriptStruct* StaticStruct<struct FSlateColor>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Styling_SlateColor_h


#define FOREACH_ENUM_ESLATECOLORSTYLINGMODE(op) \
	op(ESlateColorStylingMode::UseColor_Specified) \
	op(ESlateColorStylingMode::UseColor_ColorTable) \
	op(ESlateColorStylingMode::UseColor_Foreground) \
	op(ESlateColorStylingMode::UseColor_Foreground_Subdued) \
	op(ESlateColorStylingMode::UseColor_UseStyle) 

enum class ESlateColorStylingMode : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ESlateColorStylingMode>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
