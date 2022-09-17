// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_SlateSettings_generated_h
#error "SlateSettings.generated.h already included, missing '#pragma once' in SlateSettings.h"
#endif
#define SLATE_SlateSettings_generated_h

#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_SPARSE_DATA
#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_RPC_WRAPPERS
#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSlateSettings(); \
	friend struct Z_Construct_UClass_USlateSettings_Statics; \
public: \
	DECLARE_CLASS(USlateSettings, UObject, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/Slate"), NO_API) \
	DECLARE_SERIALIZER(USlateSettings) \
	static const TCHAR* StaticConfigName() {return TEXT("Engine");} \



#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_INCLASS \
private: \
	static void StaticRegisterNativesUSlateSettings(); \
	friend struct Z_Construct_UClass_USlateSettings_Statics; \
public: \
	DECLARE_CLASS(USlateSettings, UObject, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/Slate"), NO_API) \
	DECLARE_SERIALIZER(USlateSettings) \
	static const TCHAR* StaticConfigName() {return TEXT("Engine");} \



#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USlateSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USlateSettings) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USlateSettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USlateSettings); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API USlateSettings(USlateSettings&&); \
	NO_API USlateSettings(const USlateSettings&); \
public:


#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USlateSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API USlateSettings(USlateSettings&&); \
	NO_API USlateSettings(const USlateSettings&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USlateSettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USlateSettings); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USlateSettings)


#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_12_PROLOG
#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_SPARSE_DATA \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_RPC_WRAPPERS \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_INCLASS \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_SPARSE_DATA \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_INCLASS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h_15_ENHANCED_CONSTRUCTORS \
static_assert(false, "Unknown access specifier for GENERATED_BODY() macro in class SlateSettings."); \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SLATE_API UClass* StaticClass<class USlateSettings>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_SlateSettings_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
