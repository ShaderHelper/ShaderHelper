// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_StyleColors_generated_h
#error "StyleColors.generated.h already included, missing '#pragma once' in StyleColors.h"
#endif
#define SLATECORE_StyleColors_generated_h

#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_96_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FStyleColorList_Statics; \
	SLATECORE_API static class UScriptStruct* StaticStruct();


template<> SLATECORE_API UScriptStruct* StaticStruct<struct FStyleColorList>();

#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_110_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FStyleTheme_Statics; \
	SLATECORE_API static class UScriptStruct* StaticStruct();


template<> SLATECORE_API UScriptStruct* StaticStruct<struct FStyleTheme>();

#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_SPARSE_DATA
#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_RPC_WRAPPERS
#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSlateThemeManager(); \
	friend struct Z_Construct_UClass_USlateThemeManager_Statics; \
public: \
	DECLARE_CLASS(USlateThemeManager, UObject, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/SlateCore"), NO_API) \
	DECLARE_SERIALIZER(USlateThemeManager) \
	static const TCHAR* StaticConfigName() {return TEXT("EditorSettings");} \



#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_INCLASS \
private: \
	static void StaticRegisterNativesUSlateThemeManager(); \
	friend struct Z_Construct_UClass_USlateThemeManager_Statics; \
public: \
	DECLARE_CLASS(USlateThemeManager, UObject, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/SlateCore"), NO_API) \
	DECLARE_SERIALIZER(USlateThemeManager) \
	static const TCHAR* StaticConfigName() {return TEXT("EditorSettings");} \



#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USlateThemeManager(const FObjectInitializer& ObjectInitializer); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USlateThemeManager) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USlateThemeManager); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USlateThemeManager); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API USlateThemeManager(USlateThemeManager&&); \
	NO_API USlateThemeManager(const USlateThemeManager&); \
public:


#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API USlateThemeManager(USlateThemeManager&&); \
	NO_API USlateThemeManager(const USlateThemeManager&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USlateThemeManager); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USlateThemeManager); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(USlateThemeManager)


#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_132_PROLOG
#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_SPARSE_DATA \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_RPC_WRAPPERS \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_INCLASS \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_SPARSE_DATA \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_INCLASS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h_135_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SLATECORE_API UClass* StaticClass<class USlateThemeManager>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Styling_StyleColors_h


#define FOREACH_ENUM_ESTYLECOLOR(op) \
	op(EStyleColor::Black) \
	op(EStyleColor::Background) \
	op(EStyleColor::Title) \
	op(EStyleColor::WindowBorder) \
	op(EStyleColor::Foldout) \
	op(EStyleColor::Input) \
	op(EStyleColor::InputOutline) \
	op(EStyleColor::Recessed) \
	op(EStyleColor::Panel) \
	op(EStyleColor::Header) \
	op(EStyleColor::Dropdown) \
	op(EStyleColor::DropdownOutline) \
	op(EStyleColor::Hover) \
	op(EStyleColor::Hover2) \
	op(EStyleColor::White) \
	op(EStyleColor::White25) \
	op(EStyleColor::Highlight) \
	op(EStyleColor::Primary) \
	op(EStyleColor::PrimaryHover) \
	op(EStyleColor::PrimaryPress) \
	op(EStyleColor::Secondary) \
	op(EStyleColor::Foreground) \
	op(EStyleColor::ForegroundHover) \
	op(EStyleColor::ForegroundInverted) \
	op(EStyleColor::ForegroundHeader) \
	op(EStyleColor::Select) \
	op(EStyleColor::SelectInactive) \
	op(EStyleColor::SelectParent) \
	op(EStyleColor::SelectHover) \
	op(EStyleColor::Notifications) \
	op(EStyleColor::AccentBlue) \
	op(EStyleColor::AccentPurple) \
	op(EStyleColor::AccentPink) \
	op(EStyleColor::AccentRed) \
	op(EStyleColor::AccentOrange) \
	op(EStyleColor::AccentYellow) \
	op(EStyleColor::AccentGreen) \
	op(EStyleColor::AccentBrown) \
	op(EStyleColor::AccentBlack) \
	op(EStyleColor::AccentGray) \
	op(EStyleColor::AccentWhite) \
	op(EStyleColor::AccentFolder) \
	op(EStyleColor::Warning) \
	op(EStyleColor::Error) \
	op(EStyleColor::Success) \
	op(EStyleColor::User1) \
	op(EStyleColor::User2) \
	op(EStyleColor::User3) \
	op(EStyleColor::User4) \
	op(EStyleColor::User5) \
	op(EStyleColor::User6) \
	op(EStyleColor::User7) \
	op(EStyleColor::User8) \
	op(EStyleColor::User9) \
	op(EStyleColor::User10) \
	op(EStyleColor::User11) \
	op(EStyleColor::User12) \
	op(EStyleColor::User13) \
	op(EStyleColor::User14) \
	op(EStyleColor::User15) \
	op(EStyleColor::User16) 

enum class EStyleColor : uint8;
template<> SLATECORE_API UEnum* StaticEnum<EStyleColor>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
