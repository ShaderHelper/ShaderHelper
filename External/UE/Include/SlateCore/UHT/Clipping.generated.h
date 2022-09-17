// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_Clipping_generated_h
#error "Clipping.generated.h already included, missing '#pragma once' in Clipping.h"
#endif
#define SLATECORE_Clipping_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Layout_Clipping_h


#define FOREACH_ENUM_EWIDGETCLIPPING(op) \
	op(EWidgetClipping::Inherit) \
	op(EWidgetClipping::ClipToBounds) \
	op(EWidgetClipping::ClipToBoundsWithoutIntersecting) \
	op(EWidgetClipping::ClipToBoundsAlways) \
	op(EWidgetClipping::OnDemand) 

enum class EWidgetClipping : uint8;
template<> SLATECORE_API UEnum* StaticEnum<EWidgetClipping>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
