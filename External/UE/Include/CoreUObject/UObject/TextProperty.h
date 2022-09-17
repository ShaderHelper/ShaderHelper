// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FText, FProperty> FTextProperty_Super;

class COREUOBJECT_API FTextProperty : public FTextProperty_Super
{
	DECLARE_FIELD(FTextProperty, FTextProperty_Super, CASTCLASS_FTextProperty)

public:

	typedef FTextProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FTextProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FTextProperty_Super(InOwner, InName, InObjectFlags)
	{
	}

	FTextProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: FTextProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FTextProperty(UField* InField)
		: FTextProperty_Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of FProperty interface

	/** Generate the correct C++ code for the given text value */
	static FString GenerateCppCodeForTextValue(const FText& InValue, const FString& Indent);

	static bool Identical_Implementation(const FText& A, const FText& B, uint32 PortFlags);
};
