// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "UObject/FieldPath.h"

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FFieldPath, FProperty> FFieldPathProperty_Super;

class COREUOBJECT_API FFieldPathProperty : public FFieldPathProperty_Super
{
	DECLARE_FIELD(FFieldPathProperty, FFieldPathProperty_Super, CASTCLASS_FFieldPathProperty)

public:

	typedef FFieldPathProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FFieldPathProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FFieldPathProperty_Super(InOwner, InName, InObjectFlags)
		, PropertyClass(nullptr)
	{
	}

	FFieldPathProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, FFieldClass* InPropertyClass)
		: FFieldPathProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
		, PropertyClass(InPropertyClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FFieldPathProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	FFieldClass* PropertyClass;

	// UHT interface
	virtual FString GetCPPMacroType(FString& ExtendedTypeText) const  override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual FString GetCPPType(FString* ExtendedTypeText = nullptr, uint32 CPPExportFlags = 0) const override;
	// End of UHT interface

	// FProperty interface
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual void Serialize(FArchive& Ar) override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual bool SupportsNetSharedSerialization() const override;
	// End of FProperty interface
};