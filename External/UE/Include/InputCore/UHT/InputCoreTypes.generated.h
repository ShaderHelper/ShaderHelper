// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef INPUTCORE_InputCoreTypes_generated_h
#error "InputCoreTypes.generated.h already included, missing '#pragma once' in InputCoreTypes.h"
#endif
#define INPUTCORE_InputCoreTypes_generated_h

#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_52_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FKey_Statics; \
	static class UScriptStruct* StaticStruct();


template<> INPUTCORE_API UScriptStruct* StaticStruct<struct FKey>();

#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_SPARSE_DATA
#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_RPC_WRAPPERS
#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUInputCoreTypes(); \
	friend struct Z_Construct_UClass_UInputCoreTypes_Statics; \
public: \
	DECLARE_CLASS(UInputCoreTypes, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/InputCore"), NO_API) \
	DECLARE_SERIALIZER(UInputCoreTypes)


#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_INCLASS \
private: \
	static void StaticRegisterNativesUInputCoreTypes(); \
	friend struct Z_Construct_UClass_UInputCoreTypes_Statics; \
public: \
	DECLARE_CLASS(UInputCoreTypes, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/InputCore"), NO_API) \
	DECLARE_SERIALIZER(UInputCoreTypes)


#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInputCoreTypes(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInputCoreTypes) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInputCoreTypes); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInputCoreTypes); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UInputCoreTypes(UInputCoreTypes&&); \
	NO_API UInputCoreTypes(const UInputCoreTypes&); \
public:


#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UInputCoreTypes(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UInputCoreTypes(UInputCoreTypes&&); \
	NO_API UInputCoreTypes(const UInputCoreTypes&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UInputCoreTypes); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UInputCoreTypes); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UInputCoreTypes)


#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_762_PROLOG
#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_SPARSE_DATA \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_RPC_WRAPPERS \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_INCLASS \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_SPARSE_DATA \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_INCLASS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h_765_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> INPUTCORE_API UClass* StaticClass<class UInputCoreTypes>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_InputCore_Classes_InputCoreTypes_h


#define FOREACH_ENUM_ECONTROLLERHAND(op) \
	op(EControllerHand::Left) \
	op(EControllerHand::Right) \
	op(EControllerHand::AnyHand) \
	op(EControllerHand::Pad) \
	op(EControllerHand::ExternalCamera) \
	op(EControllerHand::Gun) \
	op(EControllerHand::HMD) \
	op(EControllerHand::Special_1) \
	op(EControllerHand::Special_2) \
	op(EControllerHand::Special_3) \
	op(EControllerHand::Special_4) \
	op(EControllerHand::Special_5) \
	op(EControllerHand::Special_6) \
	op(EControllerHand::Special_7) \
	op(EControllerHand::Special_8) \
	op(EControllerHand::Special_9) \
	op(EControllerHand::Special_10) \
	op(EControllerHand::Special_11) \
	op(EControllerHand::ControllerHand_Count) 

enum class EControllerHand : uint8;
template<> INPUTCORE_API UEnum* StaticEnum<EControllerHand>();

#define FOREACH_ENUM_ETOUCHINDEX(op) \
	op(ETouchIndex::Touch1) \
	op(ETouchIndex::Touch2) \
	op(ETouchIndex::Touch3) \
	op(ETouchIndex::Touch4) \
	op(ETouchIndex::Touch5) \
	op(ETouchIndex::Touch6) \
	op(ETouchIndex::Touch7) \
	op(ETouchIndex::Touch8) \
	op(ETouchIndex::Touch9) \
	op(ETouchIndex::Touch10) \
	op(ETouchIndex::CursorPointerIndex) \
	op(ETouchIndex::MAX_TOUCHES) 
#define FOREACH_ENUM_ECONSOLEFORGAMEPADLABELS(op) \
	op(EConsoleForGamepadLabels::None) \
	op(EConsoleForGamepadLabels::XBoxOne) \
	op(EConsoleForGamepadLabels::PS4) 
#define FOREACH_ENUM_ETOUCHTYPE(op) \
	op(ETouchType::Began) \
	op(ETouchType::Moved) \
	op(ETouchType::Stationary) \
	op(ETouchType::ForceChanged) \
	op(ETouchType::FirstMove) \
	op(ETouchType::Ended) \
	op(ETouchType::NumTypes) 
PRAGMA_ENABLE_DEPRECATION_WARNINGS
