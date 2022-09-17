// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_STableViewBase_generated_h
#error "STableViewBase.generated.h already included, missing '#pragma once' in STableViewBase.h"
#endif
#define SLATE_STableViewBase_generated_h

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Widgets_Views_STableViewBase_h


#define FOREACH_ENUM_ELISTITEMALIGNMENT(op) \
	op(EListItemAlignment::EvenlyDistributed) \
	op(EListItemAlignment::EvenlySize) \
	op(EListItemAlignment::EvenlyWide) \
	op(EListItemAlignment::LeftAligned) \
	op(EListItemAlignment::RightAligned) \
	op(EListItemAlignment::CenterAligned) \
	op(EListItemAlignment::Fill) 

enum class EListItemAlignment : uint8;
template<> SLATE_API UEnum* StaticEnum<EListItemAlignment>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
