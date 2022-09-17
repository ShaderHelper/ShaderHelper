// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_RenderingCommon_generated_h
#error "RenderingCommon.generated.h already included, missing '#pragma once' in RenderingCommon.h"
#endif
#define SLATECORE_RenderingCommon_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Rendering_RenderingCommon_h


#define FOREACH_ENUM_ECOLORVISIONDEFICIENCY(op) \
	op(EColorVisionDeficiency::NormalVision) \
	op(EColorVisionDeficiency::Deuteranope) \
	op(EColorVisionDeficiency::Protanope) \
	op(EColorVisionDeficiency::Tritanope) 

enum class EColorVisionDeficiency : uint8;
template<> SLATECORE_API UEnum* StaticEnum<EColorVisionDeficiency>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
