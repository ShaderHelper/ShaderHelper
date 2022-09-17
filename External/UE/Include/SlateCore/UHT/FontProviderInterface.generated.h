// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SLATECORE_FontProviderInterface_generated_h
#error "FontProviderInterface.generated.h already included, missing '#pragma once' in FontProviderInterface.h"
#endif
#define SLATECORE_FontProviderInterface_generated_h

#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_SPARSE_DATA
#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_RPC_WRAPPERS
#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	SLATECORE_API UFontProviderInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFontProviderInterface) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(SLATECORE_API, UFontProviderInterface); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFontProviderInterface); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	SLATECORE_API UFontProviderInterface(UFontProviderInterface&&); \
	SLATECORE_API UFontProviderInterface(const UFontProviderInterface&); \
public:


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	SLATECORE_API UFontProviderInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	SLATECORE_API UFontProviderInterface(UFontProviderInterface&&); \
	SLATECORE_API UFontProviderInterface(const UFontProviderInterface&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(SLATECORE_API, UFontProviderInterface); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFontProviderInterface); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFontProviderInterface)


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_GENERATED_UINTERFACE_BODY() \
private: \
	static void StaticRegisterNativesUFontProviderInterface(); \
	friend struct Z_Construct_UClass_UFontProviderInterface_Statics; \
public: \
	DECLARE_CLASS(UFontProviderInterface, UInterface, COMPILED_IN_FLAGS(CLASS_Abstract | CLASS_Interface), CASTCLASS_None, TEXT("/Script/SlateCore"), SLATECORE_API) \
	DECLARE_SERIALIZER(UFontProviderInterface)


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_GENERATED_BODY_LEGACY \
		PRAGMA_DISABLE_DEPRECATION_WARNINGS \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_GENERATED_UINTERFACE_BODY() \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_STANDARD_CONSTRUCTORS \
	PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_GENERATED_BODY \
	PRAGMA_DISABLE_DEPRECATION_WARNINGS \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_GENERATED_UINTERFACE_BODY() \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_ENHANCED_CONSTRUCTORS \
private: \
	PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_INCLASS_IINTERFACE_NO_PURE_DECLS \
protected: \
	virtual ~IFontProviderInterface() {} \
public: \
	typedef UFontProviderInterface UClassType; \
	typedef IFontProviderInterface ThisClass; \
	virtual UObject* _getUObject() const { return nullptr; }


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_INCLASS_IINTERFACE \
protected: \
	virtual ~IFontProviderInterface() {} \
public: \
	typedef UFontProviderInterface UClassType; \
	typedef IFontProviderInterface ThisClass; \
	virtual UObject* _getUObject() const { return nullptr; }


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_12_PROLOG
#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_20_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_SPARSE_DATA \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_RPC_WRAPPERS \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_INCLASS_IINTERFACE \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_SPARSE_DATA \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h_15_INCLASS_IINTERFACE_NO_PURE_DECLS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SLATECORE_API UClass* StaticClass<class UFontProviderInterface>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Engine_Source_Runtime_SlateCore_Public_Fonts_FontProviderInterface_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
