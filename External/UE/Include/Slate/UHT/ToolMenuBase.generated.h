// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATE_ToolMenuBase_generated_h
#error "ToolMenuBase.generated.h already included, missing '#pragma once' in ToolMenuBase.h"
#endif
#define SLATE_ToolMenuBase_generated_h

#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_23_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FCustomizedToolMenuEntry_Statics; \
	SLATE_API static class UScriptStruct* StaticStruct();


template<> SLATE_API UScriptStruct* StaticStruct<struct FCustomizedToolMenuEntry>();

#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_37_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FCustomizedToolMenuSection_Statics; \
	SLATE_API static class UScriptStruct* StaticStruct();


template<> SLATE_API UScriptStruct* StaticStruct<struct FCustomizedToolMenuSection>();

#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_51_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FCustomizedToolMenuNameArray_Statics; \
	SLATE_API static class UScriptStruct* StaticStruct();


template<> SLATE_API UScriptStruct* StaticStruct<struct FCustomizedToolMenuNameArray>();

#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_60_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FCustomizedToolMenu_Statics; \
	static class UScriptStruct* StaticStruct();


template<> SLATE_API UScriptStruct* StaticStruct<struct FCustomizedToolMenu>();

#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_SPARSE_DATA
#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_RPC_WRAPPERS
#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUToolMenuBase(); \
	friend struct Z_Construct_UClass_UToolMenuBase_Statics; \
public: \
	DECLARE_CLASS(UToolMenuBase, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/Slate"), NO_API) \
	DECLARE_SERIALIZER(UToolMenuBase)


#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_INCLASS \
private: \
	static void StaticRegisterNativesUToolMenuBase(); \
	friend struct Z_Construct_UClass_UToolMenuBase_Statics; \
public: \
	DECLARE_CLASS(UToolMenuBase, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/Slate"), NO_API) \
	DECLARE_SERIALIZER(UToolMenuBase)


#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UToolMenuBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UToolMenuBase) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UToolMenuBase); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UToolMenuBase); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UToolMenuBase(UToolMenuBase&&); \
	NO_API UToolMenuBase(const UToolMenuBase&); \
public:


#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UToolMenuBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UToolMenuBase(UToolMenuBase&&); \
	NO_API UToolMenuBase(const UToolMenuBase&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UToolMenuBase); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UToolMenuBase); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UToolMenuBase)


#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_106_PROLOG
#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_SPARSE_DATA \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_RPC_WRAPPERS \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_INCLASS \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_SPARSE_DATA \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_INCLASS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h_109_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SLATE_API UClass* StaticClass<class UToolMenuBase>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_Slate_Public_Framework_MultiBox_ToolMenuBase_h


#define FOREACH_ENUM_ECUSTOMIZEDTOOLMENUVISIBILITY(op) \
	op(ECustomizedToolMenuVisibility::None) \
	op(ECustomizedToolMenuVisibility::Visible) \
	op(ECustomizedToolMenuVisibility::Hidden) 

enum class ECustomizedToolMenuVisibility;
template<> SLATE_API UEnum* StaticEnum<ECustomizedToolMenuVisibility>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
