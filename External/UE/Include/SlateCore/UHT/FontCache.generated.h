// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_FontCache_generated_h
#error "FontCache.generated.h already included, missing '#pragma once' in FontCache.h"
#endif
#define SLATECORE_FontCache_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontCache_h


#define FOREACH_ENUM_ETEXTSHAPINGMETHOD(op) \
	op(ETextShapingMethod::Auto) \
	op(ETextShapingMethod::KerningOnly) \
	op(ETextShapingMethod::FullShaping) 

enum class ETextShapingMethod : uint8;
template<> SLATECORE_API UEnum* StaticEnum<ETextShapingMethod>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
