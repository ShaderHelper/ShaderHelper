// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealType.h: Unreal engine base type definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UnrealType.h"

// WARNING: This should always be the last include in any file that needs it (except .generated.h)
#include "UObject/UndefineUPropertyMacros.h"

#define USE_UPROPERTY_LOAD_DEFERRING (USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING && WITH_EDITORONLY_DATA)

class COREUOBJECT_API UProperty : public UField
{
	DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR(UProperty, UField, CLASS_Abstract, TEXT("/Script/CoreUObject"), CASTCLASS_FProperty, NO_API)
	DECLARE_WITHIN(UObject)

public:

	// Persistent variables.
	int32			ArrayDim;
	int32			ElementSize;
	EPropertyFlags	PropertyFlags;
	uint16			RepIndex;

	TEnumAsByte<ELifetimeCondition> BlueprintReplicationCondition;

	// In memory variables (generated during Link()).
	int32		Offset_Internal;

	FName		RepNotifyFunc;

	/** In memory only: Linked list of properties from most-derived to base **/
	UProperty*	PropertyLinkNext;
	/** In memory only: Linked list of object reference properties from most-derived to base **/
	UProperty*  NextRef;
	/** In memory only: Linked list of properties requiring destruction. Note this does not include things that will be destroyed byt he native destructor **/
	UProperty*	DestructorLinkNext;
	/** In memory only: Linked list of properties requiring post constructor initialization.**/
	UProperty*	PostConstructLinkNext;


	// Constructors.
	UProperty(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags);
	UProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags );

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	// End of UObject interface

	/**
	 * Returns the first UProperty in this property's Outer chain that does not have a UProperty for an Outer
	 */
	UProperty* GetOwnerProperty()
	{
		UProperty* Result=this;
		for (UProperty* PropBase = dynamic_cast<UProperty*>(GetOuter()); PropBase; PropBase = dynamic_cast<UProperty*>(PropBase->GetOuter()))
		{
			Result = PropBase;
		}
		return Result;
	}

	const UProperty* GetOwnerProperty() const
	{
		const UProperty* Result = this;
		for (UProperty* PropBase = dynamic_cast<UProperty*>(GetOuter()); PropBase; PropBase = dynamic_cast<UProperty*>(PropBase->GetOuter()))
		{
			Result = PropBase;
		}
		return Result;
	}

	FORCEINLINE bool HasAnyPropertyFlags( uint64 FlagsToCheck ) const
	{
		return (PropertyFlags & FlagsToCheck) != 0 || FlagsToCheck == CPF_AllFlags;
	}
	/**
	 * Used to safely check whether all of the passed in flags are set. This is required
	 * as PropertyFlags currently is a 64 bit data type and bool is a 32 bit data type so
	 * simply using PropertyFlags&CPF_MyFlagBiggerThanMaxInt won't work correctly when
	 * assigned directly to an bool.
	 *
	 * @param FlagsToCheck	Object flags to check for
	 *
	 * @return true if all of the passed in flags are set (including no flags passed in), false otherwise
	 */
	FORCEINLINE bool HasAllPropertyFlags( uint64 FlagsToCheck ) const
	{
		return ((PropertyFlags & FlagsToCheck) == FlagsToCheck);
	}

	/**
	 * Editor-only properties are those that only are used with the editor is present or cannot be removed from serialisation.
	 * Editor-only properties include: EditorOnly properties
	 * Properties that cannot be removed from serialisation are:
	 *		Boolean properties (may affect GCC_BITFIELD_MAGIC computation)
	 *		Native properties (native serialisation)
	 */
	FORCEINLINE bool IsEditorOnlyProperty() const
	{
		return (PropertyFlags & CPF_DevelopmentAssets) != 0;
	}

private:

	FORCEINLINE void* ContainerVoidPtrToValuePtrInternal(void* ContainerPtr, int32 ArrayIndex) const
	{
		check(ArrayIndex < ArrayDim);
		check(ContainerPtr);

		if (0)
		{
			// in the future, these checks will be tested if the property is NOT relative to a UClass
			check(!Cast<UClass>(GetOuter())); // Check we are _not_ calling this on a direct child property of a UClass, you should pass in a UObject* in that case
		}

		return (uint8*)ContainerPtr + Offset_Internal + ElementSize * ArrayIndex;
	}

	FORCEINLINE void* ContainerUObjectPtrToValuePtrInternal(UObject* ContainerPtr, int32 ArrayIndex) const
	{
		check(ArrayIndex < ArrayDim);
		check(ContainerPtr);

		// in the future, these checks will be tested if the property is supposed be from a UClass
		// need something for networking, since those are NOT live uobjects, just memory blocks
		check(((UObject*)ContainerPtr)->IsValidLowLevel()); // Check its a valid UObject that was passed in
		check(((UObject*)ContainerPtr)->GetClass() != NULL);
		check(GetOuter()->IsA(UClass::StaticClass())); // Check that the outer of this property is a UClass (not another property)

		// Check that the object we are accessing is of the class that contains this property
		checkf(((UObject*)ContainerPtr)->IsA((UClass*)GetOuter()), TEXT("'%s' is of class '%s' however property '%s' belongs to class '%s'")
			, *((UObject*)ContainerPtr)->GetName()
			, *((UObject*)ContainerPtr)->GetClass()->GetName()
			, *GetName()
			, *((UClass*)GetOuter())->GetName());

		if (0)
		{
			// in the future, these checks will be tested if the property is NOT relative to a UClass
			check(!GetOuter()->IsA(UClass::StaticClass())); // Check we are _not_ calling this on a direct child property of a UClass, you should pass in a UObject* in that case
		}

		return (uint8*)ContainerPtr + Offset_Internal + ElementSize * ArrayIndex;
	}

public:

	template<typename ValueType>
	FORCEINLINE ValueType* ContainerPtrToValuePtr(UObject* ContainerPtr, int32 ArrayIndex = 0) const
	{
		return (ValueType*)ContainerUObjectPtrToValuePtrInternal(ContainerPtr, ArrayIndex);
	}
	template<typename ValueType>
	FORCEINLINE ValueType* ContainerPtrToValuePtr(void* ContainerPtr, int32 ArrayIndex = 0) const
	{
		return (ValueType*)ContainerVoidPtrToValuePtrInternal(ContainerPtr, ArrayIndex);
	}
	template<typename ValueType>
	FORCEINLINE ValueType const* ContainerPtrToValuePtr(UObject const* ContainerPtr, int32 ArrayIndex = 0) const
	{
		return ContainerPtrToValuePtr<ValueType>((UObject*)ContainerPtr, ArrayIndex);
	}
	template<typename ValueType>
	FORCEINLINE ValueType const* ContainerPtrToValuePtr(void const* ContainerPtr, int32 ArrayIndex = 0) const
	{
		return ContainerPtrToValuePtr<ValueType>((void*)ContainerPtr, ArrayIndex);
	}

#if WITH_EDITORONLY_DATA

	FField* AssociatedField;

	virtual FField* GetAssociatedFField() override;
	virtual void SetAssociatedFField(FField* InField) override;
#endif // WITH_EDITORONLY_DATA
};

class COREUOBJECT_API UNumericProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UNumericProperty, UProperty, CLASS_Abstract, TEXT("/Script/CoreUObject"), CASTCLASS_FNumericProperty)

	UNumericProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
		: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{}

	UNumericProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
		:	UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{}
};

class COREUOBJECT_API UByteProperty : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UByteProperty, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FByteProperty)

public:

	// Variables.
	UEnum* Enum;

	UByteProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UEnum* InEnum = nullptr)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	, Enum(InEnum)
	{
	}

	UByteProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UEnum* InEnum = nullptr)
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	, Enum( InEnum )
	{
	}

	// UObject interface.
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface
};

class COREUOBJECT_API UInt8Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UInt8Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FInt8Property)

	UInt8Property(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
		: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UInt8Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
		: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

class COREUOBJECT_API UInt16Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UInt16Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FInt16Property)

	UInt16Property(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UInt16Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

class COREUOBJECT_API UIntProperty : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UIntProperty, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FIntProperty)

	UIntProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UIntProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

class COREUOBJECT_API UInt64Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UInt64Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FInt64Property)

	UInt64Property(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UInt64Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UUInt16Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UUInt16Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FUInt16Property)

	UUInt16Property(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UUInt16Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UUInt32Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UUInt32Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FUInt32Property)

	UUInt32Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UUInt64Property : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UUInt64Property, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FUInt64Property)

	UUInt64Property(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UUInt64Property( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UFloatProperty : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UFloatProperty, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FFloatProperty)

	UFloatProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UFloatProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UNumericProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UDoubleProperty : public UNumericProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UDoubleProperty, UNumericProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FDoubleProperty)

	UDoubleProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UDoubleProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UNumericProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

class COREUOBJECT_API UBoolProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC_NO_CTOR(UBoolProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FBoolProperty, NO_API)

	// Variables.
public:

	/** Size of the bitfield/bool property. Equal to ElementSize but used to check if the property has been properly initialized (0-8, where 0 means uninitialized). */
	uint8 FieldSize;
	/** Offset from the memeber variable to the byte of the property (0-7). */
	uint8 ByteOffset;
	/** Mask of the byte with the property value. */
	uint8 ByteMask;
	/** Mask of the field with the property value. Either equal to ByteMask or 255 in case of 'bool' type. */
	uint8 FieldMask;

	UBoolProperty(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/**
	 * Constructor.
	 *
	 * @param ECppProperty Unused.
	 * @param InOffset Offset of the property.
	 * @param InCategory Category of the property.
	 * @param InFlags Property flags.
	 * @param InBitMask Bitmask of the bitfield this property represents.
	 * @param InElementSize Sizeof of the boolean type this property represents.
	 * @param bIsNativeBool true if this property represents C++ bool type.
	 */
	UBoolProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, uint32 InBitMask, uint32 InElementSize, bool bIsNativeBool);

	/**
	 * Constructor.
	 *
	 * @param ObjectInitializer Properties.
	 * @param ECppProperty Unused.
	 * @param InOffset Offset of the property.
	 * @param InCategory Category of the property.
	 * @param InFlags Property flags.
	 * @param InBitMask Bitmask of the bitfield this property represents.
	 * @param InElementSize Sizeof of the boolean type this property represents.
	 * @param bIsNativeBool true if this property represents C++ bool type.
	 */
	UBoolProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, uint32 InBitMask, uint32 InElementSize, bool bIsNativeBool );

	// UObject interface.
	virtual void Serialize( FArchive& Ar ) override;
	// End of UObject interface

	/** 
	 * Sets the bitfield/bool type and size. 
	 * This function must be called before UBoolProperty can be used.
	 *
	 * @param InSize size of the bitfield/bool type.
	 * @param bIsNativeBool true if this property represents C++ bool type.
	 */
	void SetBoolSize( const uint32 InSize, const bool bIsNativeBool = false, const uint32 InBitMask = 0 );

	/**
	 * If the return value is true this UBoolProperty represents C++ bool type.
	 */
	FORCEINLINE bool IsNativeBool() const
	{
		return FieldMask == 0xff;
	}
};

class COREUOBJECT_API UObjectPropertyBase : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UObjectPropertyBase, UProperty, CLASS_Abstract, TEXT("/Script/CoreUObject"), CASTCLASS_FObjectPropertyBase)

public:

	// Variables.
	class UClass* PropertyClass;

	UObjectPropertyBase(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass = NULL)
		: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
		, PropertyClass(InClass)
	{}

	UObjectPropertyBase( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass=NULL )
	:	UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	,	PropertyClass( InClass )
	{}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void BeginDestroy() override;
	// End of UObject interface

#if USE_UPROPERTY_LOAD_DEFERRING
	void SetPropertyClass(UClass* NewPropertyClass);
#else
	FORCEINLINE void SetPropertyClass(UClass* NewPropertyClass) { PropertyClass = NewPropertyClass; }
#endif // USE_UPROPERTY_LOAD_DEFERRING
};

class COREUOBJECT_API UObjectProperty : public UObjectPropertyBase
{
	DECLARE_CASTED_CLASS_INTRINSIC(UObjectProperty, UObjectPropertyBase, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FObjectProperty)

	UObjectProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: UObjectPropertyBase(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InClass)
	{
	}

	UObjectProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass )
		: UObjectPropertyBase(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InClass)
	{
	}
};

class COREUOBJECT_API UWeakObjectProperty : public UObjectPropertyBase
{
	DECLARE_CASTED_CLASS_INTRINSIC(UWeakObjectProperty, UObjectPropertyBase, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FWeakObjectProperty)

	UWeakObjectProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
	: UObjectPropertyBase(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InClass)
	{
	}

	UWeakObjectProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass )
	: UObjectPropertyBase( ObjectInitializer, EC_CppProperty, InOffset, InFlags, InClass )
	{
	}
};

class COREUOBJECT_API ULazyObjectProperty : public UObjectPropertyBase
{
	DECLARE_CASTED_CLASS_INTRINSIC(ULazyObjectProperty, UObjectPropertyBase, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FLazyObjectProperty)

	ULazyObjectProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
	: UObjectPropertyBase(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InClass)
	{
	}

	ULazyObjectProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass )
	: UObjectPropertyBase(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InClass)
	{
	}
};

class COREUOBJECT_API USoftObjectProperty : public UObjectPropertyBase
{
	DECLARE_CASTED_CLASS_INTRINSIC(USoftObjectProperty, UObjectPropertyBase, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FSoftObjectProperty)

	USoftObjectProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
	: UObjectPropertyBase(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InClass)
	{}

	USoftObjectProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InClass )
	: UObjectPropertyBase(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InClass)
	{}
};

class COREUOBJECT_API UClassProperty : public UObjectProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UClassProperty, UObjectProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FClassProperty)

public:

	// Variables.
	class UClass* MetaClass;

	UClassProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass, UClass* InClassType)
		: UObjectProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InClassType ? InClassType : UClass::StaticClass())
		, MetaClass(InMetaClass)
	{
	}

	UClassProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass, UClass* InClassType)
		: UObjectProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InClassType ? InClassType : UClass::StaticClass())
	,	MetaClass( InMetaClass )
	{
	}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void BeginDestroy() override;
	// End of UObject interface

	/**
	 * Setter function for this property's MetaClass member. Favor this function 
	 * whilst loading (since, to handle circular dependencies, we defer some 
	 * class loads and use a placeholder class instead). It properly handles 
	 * deferred loading placeholder classes (so they can properly be replaced 
	 * later).
	 * 
	 * @param  NewMetaClass    The MetaClass you want this property set with.
	 */
#if USE_UPROPERTY_LOAD_DEFERRING
	void SetMetaClass(UClass* NewMetaClass);
#else
	FORCEINLINE void SetMetaClass(UClass* NewMetaClass) { MetaClass = NewMetaClass; }
#endif // USE_UPROPERTY_LOAD_DEFERRING
};

class COREUOBJECT_API USoftClassProperty : public USoftObjectProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(USoftClassProperty, USoftObjectProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FSoftClassProperty)

public:

	// Variables.
	class UClass* MetaClass;

	USoftClassProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass)
		: Super(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, UClass::StaticClass())
		, MetaClass(InMetaClass)
	{}

	USoftClassProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass )
		:	Super(ObjectInitializer, EC_CppProperty, InOffset, InFlags, UClass::StaticClass() )
		,	MetaClass( InMetaClass )
	{}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void BeginDestroy() override;
	// End of UObject interface

	/**
	 * Setter function for this property's MetaClass member. Favor this function 
	 * whilst loading (since, to handle circular dependencies, we defer some 
	 * class loads and use a placeholder class instead). It properly handles 
	 * deferred loading placeholder classes (so they can properly be replaced 
	 * later).
	 * 
	 * @param  NewMetaClass    The MetaClass you want this property set with.
	 */
#if USE_UPROPERTY_LOAD_DEFERRING
	void SetMetaClass(UClass* NewMetaClass);
#else
	FORCEINLINE void SetMetaClass(UClass* NewMetaClass) { MetaClass = NewMetaClass; }
#endif // USE_UPROPERTY_LOAD_DEFERRING
};

class COREUOBJECT_API UInterfaceProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UInterfaceProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FInterfaceProperty)

public:

	/** The native interface class that this interface property refers to */
	class	UClass*		InterfaceClass;

	UInterfaceProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InInterfaceClass)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, (InFlags & ~CPF_InterfaceClearMask))
	, InterfaceClass(InInterfaceClass)
	{
	}

	UInterfaceProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UClass* InInterfaceClass )
	: UProperty( ObjectInitializer, EC_CppProperty, InOffset, (InFlags & ~CPF_InterfaceClearMask) )
	, InterfaceClass( InInterfaceClass )
	{
	}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void BeginDestroy() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface

	/**
	 * Setter function for this property's InterfaceClass member. Favor this 
	 * function whilst loading (since, to handle circular dependencies, we defer 
	 * some class loads and use a placeholder class instead). It properly 
	 * handles deferred loading placeholder classes (so they can properly be 
	 * replaced later).
	 *  
	 * @param  NewInterfaceClass    The InterfaceClass you want this property set with.
	 */
#if USE_UPROPERTY_LOAD_DEFERRING
	void SetInterfaceClass(UClass* NewInterfaceClass);
#else
	FORCEINLINE void SetInterfaceClass(UClass* NewInterfaceClass) { InterfaceClass = NewInterfaceClass; }
#endif // USE_UPROPERTY_LOAD_DEFERRING
};

class COREUOBJECT_API UNameProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UNameProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FNameProperty)
public:

	UNameProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UNameProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags )
	{
	}
};

class COREUOBJECT_API UStrProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UStrProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FStrProperty)
public:

	UStrProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UStrProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

class COREUOBJECT_API UArrayProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UArrayProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FArrayProperty)

public:

	// Variables.
	UProperty* Inner;

	UArrayProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UArrayProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags )
	: UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface
};

class COREUOBJECT_API UMapProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UMapProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FMapProperty)

public:

	// Properties representing the key type and value type of the contained pairs
	UProperty*       KeyProp;
	UProperty*       ValueProp;
	FScriptMapLayout MapLayout;

	UMapProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags);

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface
};

class COREUOBJECT_API USetProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(USetProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FSetProperty)

public:

	// Properties representing the key type and value type of the contained pairs
	UProperty*       ElementProp;
	FScriptSetLayout SetLayout;

	USetProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags);

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface
};

class COREUOBJECT_API UStructProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UStructProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FStructProperty)

public:

	// Variables.
	class UScriptStruct* Struct;

	UStructProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UScriptStruct* InStruct);
	UStructProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UScriptStruct* InStruct );

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface
};

class COREUOBJECT_API UDelegateProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UDelegateProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FDelegateProperty)

public:

	/** Points to the source delegate function (the function declared with the delegate keyword) used in the declaration of this delegate property. */
	UFunction* SignatureFunction;

	UDelegateProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = NULL)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	, SignatureFunction(InSignatureFunction)
	{
	}

	UDelegateProperty( const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = NULL )
	: UProperty( ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	, SignatureFunction(InSignatureFunction)
	{
	}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void BeginDestroy() override;
	// End of UObject interface
};

class COREUOBJECT_API UMulticastDelegateProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UMulticastDelegateProperty, UProperty, CLASS_Abstract, TEXT("/Script/CoreUObject"), CASTCLASS_FMulticastDelegateProperty)

public:

	/** Points to the source delegate function (the function declared with the delegate keyword) used in the declaration of this delegate property. */
	UFunction* SignatureFunction;

	UMulticastDelegateProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
		: UProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags)
		, SignatureFunction(InSignatureFunction)
	{
	}

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void BeginDestroy() override;
	// End of UObject interface
};

class COREUOBJECT_API UMulticastInlineDelegateProperty : public UMulticastDelegateProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UMulticastInlineDelegateProperty, UMulticastDelegateProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FMulticastInlineDelegateProperty)

public:

	UMulticastInlineDelegateProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
	: UMulticastDelegateProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InSignatureFunction)
	{
	}

	UMulticastInlineDelegateProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
	: UMulticastDelegateProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InSignatureFunction)
	{
	}
};

class COREUOBJECT_API UMulticastSparseDelegateProperty : public UMulticastDelegateProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UMulticastSparseDelegateProperty, UMulticastDelegateProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FMulticastSparseDelegateProperty)

public:

	UMulticastSparseDelegateProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
	: UMulticastDelegateProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags, InSignatureFunction)
	{
	}

	UMulticastSparseDelegateProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
	: UMulticastDelegateProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags, InSignatureFunction)
	{
	}
};

class COREUOBJECT_API UEnumProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UEnumProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FEnumProperty)

public:
	UEnumProperty(const FObjectInitializer& ObjectInitializer, UEnum* InEnum);
	UEnumProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags, UEnum* InEnum);

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface

	UNumericProperty* UnderlyingProp; // The property which represents the underlying type of the enum
	UEnum* Enum; // The enum represented by this property
};

class COREUOBJECT_API UTextProperty : public UProperty
{
	DECLARE_CASTED_CLASS_INTRINSIC(UTextProperty, UProperty, 0, TEXT("/Script/CoreUObject"), CASTCLASS_FTextProperty)

public:

	UTextProperty(ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UProperty(FObjectInitializer::Get(), EC_CppProperty, InOffset, InFlags)
	{
	}

	UTextProperty(const FObjectInitializer& ObjectInitializer, ECppProperty, int32 InOffset, EPropertyFlags InFlags)
	: UProperty(ObjectInitializer, EC_CppProperty, InOffset, InFlags)
	{
	}
};

// WARNING: This should always be the last include in any file that needs it (except .generated.h)
#include "UObject/DefineUPropertyMacros.h"