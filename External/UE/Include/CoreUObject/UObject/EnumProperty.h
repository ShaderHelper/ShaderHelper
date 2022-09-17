// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "UObject/ObjectMacros.h"

class UEnum;
class FNumericProperty;

class COREUOBJECT_API FEnumProperty : public FProperty
{
	DECLARE_FIELD(FEnumProperty, FProperty, CASTCLASS_FEnumProperty)

public:
	FEnumProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);
	FEnumProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, UEnum* InEnum);
	FEnumProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UEnum* InEnum);
#if WITH_EDITORONLY_DATA
	explicit FEnumProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA
	virtual ~FEnumProperty();

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;
	virtual FField* GetInnerFieldByName(const FName& InName) override;
	virtual void GetInnerFields(TArray<FField*>& OutFields) override;

	// UField interface
	virtual void AddCppProperty(FProperty* Property) override;
	// End of UField interface

	// FProperty interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual int32 GetMinAlignment() const override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	/**
	 * Set the UEnum of this property.
	 * @note May only be called once to lazily initialize the property when using the default constructor.
	 */
	FORCEINLINE void SetEnum(UEnum* InEnum)
	{
		checkf(!Enum, TEXT("FEnumProperty enum may only be set once"));
		Enum = InEnum;
	}

	/**
	 * Returns a pointer to the UEnum of this property.
	 */
	FORCEINLINE UEnum* GetEnum() const
	{
		return Enum;
	}

	/**
	 * Returns the numeric property which represents the integral type of the enum.
	 */
	FORCEINLINE FNumericProperty* GetUnderlyingProperty() const
	{
		return UnderlyingProp;
	}

	// Returns the number of bits required by NetSerializeItem to encode this enum, based on the maximum value
	uint64 GetMaxNetSerializeBits() const;

private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;

#if HACK_HEADER_GENERATOR
public:
#endif
	FNumericProperty* UnderlyingProp; // The property which represents the underlying type of the enum
	UEnum* Enum; // The enum represented by this property
};
