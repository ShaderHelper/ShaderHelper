// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealType.h: Unreal engine base type definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"

#include "Concepts/GetTypeHashable.h"
#include "Containers/List.h"
#include "Containers/ArrayView.h"
#include "Serialization/SerializedPropertyScope.h"
#include "Serialization/MemoryImage.h"
#include "Templates/Casts.h"
#include "Templates/Greater.h"
#include "Templates/IsFloatingPoint.h"
#include "Templates/IsIntegral.h"
#include "Templates/IsSigned.h"
#include "Templates/Models.h"
#include "UObject/Class.h"
#include "UObject/CoreNetTypes.h"
#include "UObject/LazyObjectPtr.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectPtr.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/PropertyTag.h"
#include "UObject/ScriptInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SparseDelegate.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/Field.h"

// WARNING: This should always be the last include in any file that needs it (except .generated.h)
#include "UObject/UndefineUPropertyMacros.h"

class FBlake3;
class UPropertyWrapper;

COREUOBJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogType, Log, All);

/*-----------------------------------------------------------------------------
	FProperty.
-----------------------------------------------------------------------------*/

enum EPropertyExportCPPFlags
{
	/** Indicates that there are no special C++ export flags */
	CPPF_None						=	0x00000000,
	/** Indicates that we are exporting this property's CPP text for an optional parameter value */
	CPPF_OptionalValue				=	0x00000001,
	/** Indicates that we are exporting this property's CPP text for an argument or return value */
	CPPF_ArgumentOrReturnValue		=	0x00000002,
	/** Indicates thet we are exporting this property's CPP text for C++ definition of a function. */
	CPPF_Implementation				=	0x00000004,
	/** Indicates thet we are exporting this property's CPP text with an custom type name */
	CPPF_CustomTypeName				=	0x00000008,
	/** No 'const' keyword */
	CPPF_NoConst					=	0x00000010,
	/** No reference '&' sign */
	CPPF_NoRef						=	0x00000020,
	/** No static array [%d] */
	CPPF_NoStaticArray				=	0x00000040,
	/** Blueprint compiler generated C++ code */
	CPPF_BlueprintCppBackend		=	0x00000080,
};

namespace EExportedDeclaration
{
	enum Type
	{
		Local,
		Member,
		Parameter,
		/** Type and mane are separated by comma */
		MacroParameter, 
	};
}

enum class EConvertFromTypeResult
{
	UseSerializeItem,
	CannotConvert,
	Converted
};

enum class EPropertyObjectReferenceType : uint32
{
	None = 0,
	Strong = 1 << 0,
	Weak = 1 << 1
};
ENUM_CLASS_FLAGS(EPropertyObjectReferenceType);

namespace UEProperty_Private { class FProperty_DoNotUse; }

/**
 * Helper class that calculates the maximum stack entry size required by Garbage Collector Token Stream for each class 
 */
class COREUOBJECT_API FGCStackSizeHelper
{
	int32 MaxStackSize = 0;
	/** Current property stack. Includes properties nested inside of struct member variables */
	TArray<const FProperty*> PropertyStack;
	
public:

	FGCStackSizeHelper() = default;
	FGCStackSizeHelper(const FGCStackSizeHelper& Other) = delete;
	FGCStackSizeHelper& operator=(const FGCStackSizeHelper& Other) = delete;

	~FGCStackSizeHelper()
	{
		// Sanity check to make sure every Push() was followed by a Pop()
		check(PropertyStack.Num() == 0);
	}

	void Push(const FProperty* InProperty)
	{
		PropertyStack.Push(InProperty);
		MaxStackSize = FMath::Max(PropertyStack.Num(), MaxStackSize);
	}

	void Pop()
	{
		PropertyStack.Pop();
	}

	const TArray<const FProperty*>& GetPropertyStack() const
	{
		return PropertyStack;
	}

	/** Converts the interal property stack to a string representing the current property path (Member.StructMember.InnerStructMember) */
	FString GetPropertyPath() const;

	/** 
	 * Converts a property path constructed with GetPropertyPath() to an array of properties (from the outermost to the innermost) *
	 * @param ObjectClass Class that defines the outermost property
	 * @param InPropertyPath Property path
	 * @param OutProperties resulting array
	 * @returns true if the conversion was successful, false otherwise
	 */
	static bool ConvertPathToProperties(UClass* ObjectClass, const FName& InPropertyPath, TArray<FProperty*>& OutProperties);

	int32 GetMaxStackSize() const
	{
		return MaxStackSize;
	}
};

/** Helper class to push and pop FGCStackSizeHelper stack inside of the current scope */
class FGCStackSizeHelperScope
{
	FGCStackSizeHelper& StackSizeHelper;
public:
	explicit FGCStackSizeHelperScope(FGCStackSizeHelper& InHelper, FProperty* InProperty)
		: StackSizeHelper(InHelper)
	{		
		StackSizeHelper.Push(InProperty);
	}

	FGCStackSizeHelperScope(const FGCStackSizeHelperScope& Other) = delete;
	FGCStackSizeHelperScope& operator=(const FGCStackSizeHelperScope& Other) = delete;

	~FGCStackSizeHelperScope()
	{		
		StackSizeHelper.Pop();
	}
};

//
// An UnrealScript variable.
//
class COREUOBJECT_API FProperty : public FField
{
	DECLARE_FIELD(FProperty, FField, CASTCLASS_FProperty)

	// Persistent variables.
	int32			ArrayDim;
	int32			ElementSize;
	EPropertyFlags	PropertyFlags;
	uint16			RepIndex;

private:
	TEnumAsByte<ELifetimeCondition> BlueprintReplicationCondition;

	// In memory variables (generated during Link()).
	int32		Offset_Internal;

public:
	FName		RepNotifyFunc;

	/** In memory only: Linked list of properties from most-derived to base **/
	FProperty*	PropertyLinkNext;
	/** In memory only: Linked list of object reference properties from most-derived to base **/
	FProperty*  NextRef;
	/** In memory only: Linked list of properties requiring destruction. Note this does not include things that will be destroyed byt he native destructor **/
	FProperty*	DestructorLinkNext;
	/** In memory only: Linked list of properties requiring post constructor initialization.**/
	FProperty*	PostConstructLinkNext;

public:
	// Constructors.
	FProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);
	FProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags);
#if WITH_EDITORONLY_DATA
	explicit FProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	// End of UObject interface

	// FField interface
	virtual void PostDuplicate(const FField& InField) override;

	/** parses and imports a text definition of a single property's value (if array, may be an individual element)
	 * also includes parsing of special operations for array properties (Add/Remove/RemoveIndex/Empty)
	 * @param Str	the string to parse
	 * @param DestData	base location the parsed property should place its data (DestData + ParsedProperty->Offset)
	 * @param ObjectStruct	the struct containing the valid fields
	 * @param SubobjectOuter	owner of DestData and any subobjects within it
	 * @param PortFlags	property import flags
	 * @param Warn	output device for any error messages
	 * @param DefinedProperties (out)	list of properties/indices that have been parsed by previous calls, so duplicate definitions cause an error
	 * @return pointer to remaining text in the stream (even on failure, but on failure it may not be advanced past the entire key/value pair)
	 */
	static const TCHAR* ImportSingleProperty( const TCHAR* Str, void* DestData, class UStruct* ObjectStruct, UObject* SubobjectOuter, int32 PortFlags,
											FOutputDevice* Warn, TArray<struct FDefinedProperty>& DefinedProperties );

	/** Gets a redirected property name, will return NAME_None if no redirection was found */
	static FName FindRedirectedPropertyName(UStruct* ObjectStruct, FName OldName);

	// UHT interface
	void ExportCppDeclaration(FOutputDevice& Out, EExportedDeclaration::Type DeclarationType, const TCHAR* ArrayDimOverride = NULL, uint32 AdditionalExportCPPFlags = 0
		, bool bSkipParameterName = false, const FString* ActualCppType = nullptr, const FString* ActualExtendedType = nullptr, const FString* ActualParameterName = nullptr) const;
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const;
	virtual bool PassCPPArgsByRef() const;

	/**
	 * Returns the C++ name of the property, including the _DEPRECATED suffix if the 
	 * property is deprecated.
	 *
	 * @return C++ name of property
	 */
	FString GetNameCPP() const;

	/**
	 * Returns the text to use for exporting this property to header file.
	 *
	 * @param	ExtendedTypeText	for property types which use templates, will be filled in with the type
	 * @param	CPPExportFlags		flags for modifying the behavior of the export
	 */
	virtual FString GetCPPType( FString* ExtendedTypeText=NULL, uint32 CPPExportFlags=0 ) const PURE_VIRTUAL(FProperty::GetCPPType,return TEXT(""););

	virtual FString GetCPPTypeForwardDeclaration() const PURE_VIRTUAL(FProperty::GetCPPTypeForwardDeclaration, return TEXT(""););
	// End of UHT interface

#if WITH_EDITORONLY_DATA
	/** Gets the wrapper object for this property or creates one if it doesn't exist yet */
	UPropertyWrapper* GetUPropertyWrapper();
#endif
private:
	/** Set the alignment offset for this property 
	 * @return the size of the structure including this newly added property
	*/
	int32 SetupOffset();

protected:
	friend class FMapProperty;
	friend class UEProperty_Private::FProperty_DoNotUse;

	/** Set the alignment offset for this property - added for FMapProperty */
	void SetOffset_Internal(int32 NewOffset);

	/**
	 * Initializes internal state.
	 */
	void Init();

public:
	/** Return offset of property from container base. */
	FORCEINLINE int32 GetOffset_ForDebug() const
	{
		return Offset_Internal;
	}
	/** Return offset of property from container base. */
	FORCEINLINE int32 GetOffset_ForUFunction() const
	{
		return Offset_Internal;
	}
	/** Return offset of property from container base. */
	FORCEINLINE int32 GetOffset_ForGC() const
	{
		return Offset_Internal;
	}
	/** Return offset of property from container base. */
	FORCEINLINE int32 GetOffset_ForInternal() const
	{
		return Offset_Internal;
	}
	/** Return offset of property from container base. */
	FORCEINLINE int32 GetOffset_ReplaceWith_ContainerPtrToValuePtr() const
	{
		return Offset_Internal;
	}

	void LinkWithoutChangingOffset(FArchive& Ar)
	{
		LinkInternal(Ar);
	}

	int32 Link(FArchive& Ar)
	{
		LinkInternal(Ar);
		return SetupOffset();
	}

protected:
	virtual void LinkInternal(FArchive& Ar);
public:

	/**
	* Allows a property to implement backwards compatibility handling for tagged properties
	* 
	* @param	Tag			property tag of the loading data
	* @param	Ar			the archive the data is being loaded from
	* @param	Data		
	* @param	DefaultsStruct 
	*
	* @return	A state which tells the tagged property system how the property dealt with the data.
	*			Converted:        the function has handled the tag.
	*			CannotConvert:    the tag is not something that the property can convert.
	*			UseSerializeItem: no conversion was done on the property - this can mean that the tag is correct and normal serialization applies or that the tag is incompatible.
	*/
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct);

	/**
	 * Determines whether the property values are identical.
	 * 
	 * @param	A			property data to be compared, already offset
	 * @param	B			property data to be compared, already offset
	 * @param	PortFlags	allows caller more control over how the property values are compared
	 *
	 * @return	true if the property values are identical
	 */
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags=0 ) const PURE_VIRTUAL(FProperty::Identical,return false;);

	/**
	 * Determines whether the property values are identical.
	 * 
	 * @param	A			property container of data to be compared, NOT offset
	 * @param	B			property container of data to be compared, NOT offset
	 * @param	PortFlags	allows caller more control over how the property values are compared
	 *
	 * @return	true if the property values are identical
	 */
	bool Identical_InContainer( const void* A, const void* B, int32 ArrayIndex = 0, uint32 PortFlags=0 ) const
	{
		return Identical( ContainerPtrToValuePtr<void>(A, ArrayIndex), B ? ContainerPtrToValuePtr<void>(B, ArrayIndex) : NULL, PortFlags );
	}

	/**
	 * Serializes the property with the struct's data residing in Data.
	 *
	 * @param	Ar				the archive to use for serialization
	 * @param	Data			pointer to the location of the beginning of the struct's property data
	 * @param	ArrayIdx		if not -1 (default), only this array slot will be serialized
	 */
	void SerializeBinProperty( FStructuredArchive::FSlot Slot, void* Data, int32 ArrayIdx = -1 )
	{
		FStructuredArchive::FStream Stream = Slot.EnterStream();
		if( ShouldSerializeValue(Slot.GetUnderlyingArchive()) )
		{
			const int32 LoopMin = ArrayIdx < 0 ? 0 : ArrayIdx;
			const int32 LoopMax = ArrayIdx < 0 ? ArrayDim : ArrayIdx + 1;
			for (int32 Idx = LoopMin; Idx < LoopMax; Idx++)
			{
				// Keep setting the property in case something inside of SerializeItem changes it
				FSerializedPropertyScope SerializedProperty(Slot.GetUnderlyingArchive(), this);
				SerializeItem(Stream.EnterElement(), ContainerPtrToValuePtr<void>(Data, Idx));
			}
		}
	}
	/**
	 * Serializes the property with the struct's data residing in Data, unless it matches the default
	 *
	 * @param	Ar				the archive to use for serialization
	 * @param	Data			pointer to the location of the beginning of the struct's property data
	 * @param	DefaultData		pointer to the location of the beginning of the data that should be compared against
	 * @param	DefaultStruct	struct corresponding to the block of memory located at DefaultData 
	 */
	void SerializeNonMatchingBinProperty( FStructuredArchive::FSlot Slot, void* Data, void const* DefaultData, UStruct* DefaultStruct)
	{
		FArchive& UnderlyingArchive = Slot.GetUnderlyingArchive();
		FStructuredArchive::FStream Stream = Slot.EnterStream();

		if( ShouldSerializeValue(UnderlyingArchive) )
		{
			for (int32 Idx = 0; Idx < ArrayDim; Idx++)
			{
				void *Target = ContainerPtrToValuePtr<void>(Data, Idx);
				void const* Default = ContainerPtrToValuePtrForDefaults<void>(DefaultStruct, DefaultData, Idx);
				if ( !Identical(Target, Default, UnderlyingArchive.GetPortFlags()) )
				{
					FSerializedPropertyScope SerializedProperty(UnderlyingArchive, this);
					SerializeItem( Stream.EnterElement(), Target, Default );
				}
			}
		}
	}

	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults = NULL) const PURE_VIRTUAL(FProperty::SerializeItem, );
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const;
	virtual bool SupportsNetSharedSerialization() const;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope = NULL ) const PURE_VIRTUAL(FProperty::ExportTextItem,);
	const TCHAR* ImportText( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText = (FOutputDevice*)GWarn ) const
	{
		if ( !ValidateImportFlags(PortFlags,ErrorText) || Buffer == NULL )
		{
			return NULL;
		}
		PortFlags |= EPropertyPortFlags::PPF_UseDeprecatedProperties; // Imports should always process deprecated properties
		return ImportText_Internal( Buffer, Data, PortFlags, OwnerObject, ErrorText );
	}

#if WITH_EDITORONLY_DATA
	/**
	 * Updates the given HashBuilder with name and type information of this Property.
	 * Contract: the hashed data is different from any property that serializes differently in Tagged Property Serialization.
	 * If necessary to follow the contract, subclasses should override and add further information after calling
	 * Super::AppendSchemaHash. e.g. FStructProperty needs to append the schema hash of its UStruct.
	 * 
	 * @param HashBuilder The builder to Update with property information
	 * @param bSkipEditorOnly Used by subclasses with sub- properties that may be editor-only.
	 *                        If true, sub- properties that are editor-only should not be appended to the hash.
	 *                        This property's base data is appended without regard for bSkipEditorOnly.
	 */
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const;
#endif
protected:
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const PURE_VIRTUAL(FProperty::ImportText,return NULL;);
public:
	
	bool ExportText_Direct( FString& ValueStr, const void* Data, const void* Delta, UObject* Parent, int32 PortFlags, UObject* ExportRootScope = NULL ) const;
	FORCEINLINE bool ExportText_InContainer( int32 Index, FString& ValueStr, const void* Data, const void* Delta, UObject* Parent, int32 PortFlags, UObject* ExportRootScope = NULL ) const
	{
		return ExportText_Direct(ValueStr, ContainerPtrToValuePtr<void>(Data, Index), ContainerPtrToValuePtrForDefaults<void>(NULL, Delta, Index), Parent, PortFlags, ExportRootScope);
	}

private:

	FORCEINLINE void* ContainerVoidPtrToValuePtrInternal(void* ContainerPtr, int32 ArrayIndex) const
	{
		checkf((ArrayIndex >= 0) && (ArrayIndex < ArrayDim), TEXT("Array index out of bounds: %i from an array of size %i"), ArrayIndex, ArrayDim);
		check(ContainerPtr);

		if (0)
		{
			// in the future, these checks will be tested if the property is NOT relative to a UClass
			check(!GetOwner<UClass>()); // Check we are _not_ calling this on a direct child property of a UClass, you should pass in a UObject* in that case
		}

		return (uint8*)ContainerPtr + Offset_Internal + ElementSize * ArrayIndex;
	}

	FORCEINLINE void* ContainerUObjectPtrToValuePtrInternal(UObject* ContainerPtr, int32 ArrayIndex) const
	{
		checkf((ArrayIndex >= 0) && (ArrayIndex < ArrayDim), TEXT("Array index out of bounds: %i from an array of size %i"), ArrayIndex, ArrayDim);
		check(ContainerPtr);

		// in the future, these checks will be tested if the property is supposed be from a UClass
		// need something for networking, since those are NOT live uobjects, just memory blocks
		check(((UObject*)ContainerPtr)->IsValidLowLevel()); // Check its a valid UObject that was passed in
		check(((UObject*)ContainerPtr)->GetClass() != NULL);
		check(GetOwner<UClass>()); // Check that the outer of this property is a UClass (not another property)

		// Check that the object we are accessing is of the class that contains this property
		checkf(((UObject*)ContainerPtr)->IsA(GetOwner<UClass>()), TEXT("'%s' is of class '%s' however property '%s' belongs to class '%s'")
			, *((UObject*)ContainerPtr)->GetName()
			, *((UObject*)ContainerPtr)->GetClass()->GetName()
			, *GetName()
			, *GetOwner<UClass>()->GetName());

		if (0)
		{
			// in the future, these checks will be tested if the property is NOT relative to a UClass
			check(!GetOwner<UClass>()); // Check we are _not_ calling this on a direct child property of a UClass, you should pass in a UObject* in that case
		}

		return (uint8*)ContainerPtr + Offset_Internal + ElementSize * ArrayIndex;
	}

public:

	/** 
	 *	Get the pointer to property value in a supplied 'container'. 
	 *	You can _only_ call this function on a UObject* or a uint8*. If the property you want is a 'top level' UObject property, you _must_
	 *	call the function passing in a UObject* and not a uint8*. There are checks inside the function to vertify this.
	 *	@param	ContainerPtr			UObject* or uint8* to container of property value
	 *	@param	ArrayIndex				In array case, index of array element we want
	 */
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

	// Default variants, these accept and return NULL, and also check the property against the size of the container. 
	// If we copying from a baseclass (like for a CDO), then this will give NULL for a property that doesn't belong to the baseclass
	template<typename ValueType>
	FORCEINLINE ValueType* ContainerPtrToValuePtrForDefaults(UStruct* ContainerClass, UObject* ContainerPtr, int32 ArrayIndex = 0) const
	{
		if (ContainerPtr && IsInContainer(ContainerClass))
		{
			return ContainerPtrToValuePtr<ValueType>(ContainerPtr, ArrayIndex);
		}
		return NULL;
	}
	template<typename ValueType>
	FORCEINLINE ValueType* ContainerPtrToValuePtrForDefaults(UStruct* ContainerClass, void* ContainerPtr, int32 ArrayIndex = 0) const
	{
		if (ContainerPtr && IsInContainer(ContainerClass))
		{
			return ContainerPtrToValuePtr<ValueType>(ContainerPtr, ArrayIndex);
		}
		return NULL;
	}
	template<typename ValueType>
	FORCEINLINE ValueType const* ContainerPtrToValuePtrForDefaults(UStruct* ContainerClass, UObject const* ContainerPtr, int32 ArrayIndex = 0) const
	{
		if (ContainerPtr && IsInContainer(ContainerClass))
		{
			return ContainerPtrToValuePtr<ValueType>(ContainerPtr, ArrayIndex);
		}
		return NULL;
	}
	template<typename ValueType>
	FORCEINLINE ValueType const* ContainerPtrToValuePtrForDefaults(UStruct* ContainerClass, void const* ContainerPtr, int32 ArrayIndex = 0) const
	{
		if (ContainerPtr && IsInContainer(ContainerClass))
		{
			return ContainerPtrToValuePtr<ValueType>(ContainerPtr, ArrayIndex);
		}
		return NULL;
	}
	/** See if the offset of this property is below the supplied container size */
	FORCEINLINE bool IsInContainer(int32 ContainerSize) const
	{
		return Offset_Internal + GetSize() <= ContainerSize;
	}
	/** See if the offset of this property is below the supplied container size */
	FORCEINLINE bool IsInContainer(UStruct* ContainerClass) const
	{
		return Offset_Internal + GetSize() <= (ContainerClass ? ContainerClass->GetPropertiesSize() : MAX_int32);
	}

	/**
	 * Copy the value for a single element of this property.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET + INDEX * SIZE, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 *									INDEX = the index that you want to copy.  for properties which are not arrays, this should always be 0
	 *									SIZE = the ElementSize of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 * @param	InstancingParams	contains information about instancing (if any) to perform
	 */
	FORCEINLINE void CopySingleValue( void* Dest, void const* Src ) const
	{
		if(Dest != Src)
		{
			if (PropertyFlags & CPF_IsPlainOldData)
			{
				FMemory::Memcpy( Dest, Src, ElementSize );
			}
			else
			{
				CopyValuesInternal(Dest, Src, 1);
			}
		}
	}

	/**
	 * Returns the hash value for an element of this property.
	 */
	uint32 GetValueTypeHash(const void* Src) const;

protected:
	virtual void CopyValuesInternal( void* Dest, void const* Src, int32 Count  ) const;
	virtual uint32 GetValueTypeHashInternal(const void* Src) const;

public:
	/**
	 * Copy the value for all elements of this property.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 * @param	InstancingParams	contains information about instancing (if any) to perform
	 */
	FORCEINLINE void CopyCompleteValue( void* Dest, void const* Src ) const
	{
		if(Dest != Src)
		{
			if (PropertyFlags & CPF_IsPlainOldData)
			{
				FMemory::Memcpy( Dest, Src, ElementSize * ArrayDim );
			}
			else
			{
				CopyValuesInternal(Dest, Src, ArrayDim);
			}
		}
	}
	FORCEINLINE void CopyCompleteValue_InContainer( void* Dest, void const* Src ) const
	{
		return CopyCompleteValue(ContainerPtrToValuePtr<void>(Dest), ContainerPtrToValuePtr<void>(Src));
	}

	/**
	 * Copy the value for a single element of this property. To the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET + INDEX * SIZE, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 *									INDEX = the index that you want to copy.  for properties which are not arrays, this should always be 0
	 *									SIZE = the ElementSize of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopySingleValueToScriptVM( void* Dest, void const* Src ) const;

	/**
	 * Copy the value for all elements of this property. To the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopyCompleteValueToScriptVM( void* Dest, void const* Src ) const;

	/**
	 * Copy the value for a single element of this property. From the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET + INDEX * SIZE, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 *									INDEX = the index that you want to copy.  for properties which are not arrays, this should always be 0
	 *									SIZE = the ElementSize of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopySingleValueFromScriptVM( void* Dest, void const* Src ) const;

	/**
	 * Copy the value for all elements of this property. From the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopyCompleteValueFromScriptVM( void* Dest, void const* Src ) const;

	/**
	 * Zeros the value for this property. The existing data is assumed valid (so for example this calls FString::Empty)
	 * This only does one item and not the entire fixed size array.
	 *
	 * @param	Data		the address of the value for this property that should be cleared.
	 */
	FORCEINLINE void ClearValue( void* Data ) const
	{
		if (HasAllPropertyFlags(CPF_NoDestructor | CPF_ZeroConstructor))
		{
			FMemory::Memzero( Data, ElementSize );
		}
		else
		{
			ClearValueInternal((uint8*)Data);
		}
	}
	/**
	 * Zeros the value for this property. The existing data is assumed valid (so for example this calls FString::Empty)
	 * This only does one item and not the entire fixed size array.
	 *
	 * @param	Data		the address of the container of the value for this property that should be cleared.
	 */
	FORCEINLINE void ClearValue_InContainer( void* Data, int32 ArrayIndex = 0 ) const
	{
		if (HasAllPropertyFlags(CPF_NoDestructor | CPF_ZeroConstructor))
		{
			FMemory::Memzero( ContainerPtrToValuePtr<void>(Data, ArrayIndex), ElementSize );
		}
		else
		{
			ClearValueInternal(ContainerPtrToValuePtr<uint8>(Data, ArrayIndex));
		}
	}
protected:
	virtual void ClearValueInternal( void* Data ) const;
public:
	/**
	 * Destroys the value for this property. The existing data is assumed valid (so for example this calls FString::Empty)
	 * This does the entire fixed size array.
	 *
	 * @param	Dest		the address of the value for this property that should be destroyed.
	 */
	FORCEINLINE void DestroyValue( void* Dest ) const
	{
		if (!(PropertyFlags & CPF_NoDestructor))
		{
			DestroyValueInternal(Dest);
		}
	}
	/**
	 * Destroys the value for this property. The existing data is assumed valid (so for example this calls FString::Empty)
	 * This does the entire fixed size array.
	 *
	 * @param	Dest		the address of the container containing the value that should be destroyed.
	 */
	FORCEINLINE void DestroyValue_InContainer( void* Dest ) const
	{
		if (!(PropertyFlags & CPF_NoDestructor))
		{
			DestroyValueInternal(ContainerPtrToValuePtr<void>(Dest));
		}
	}
protected:
	virtual void DestroyValueInternal( void* Dest ) const;
public:

	/**
	 * Zeros, copies from the default, or calls the constructor for on the value for this property. 
	 * The existing data is assumed invalid (so for example this might indirectly call FString::FString,
	 * This will do the entire fixed size array.
	 *
	 * @param	Dest		the address of the value for this property that should be cleared.
	 */
	FORCEINLINE void InitializeValue( void* Dest ) const
	{
		if (PropertyFlags & CPF_ZeroConstructor)
		{
			FMemory::Memzero(Dest,ElementSize * ArrayDim);
		}
		else
		{
			InitializeValueInternal(Dest);
		}
	}
	/**
	 * Zeros, copies from the default, or calls the constructor for on the value for this property. 
	 * The existing data is assumed invalid (so for example this might indirectly call FString::FString,
	 * This will do the entire fixed size array.
	 *
	 * @param	Dest		the address of the container of value for this property that should be cleared.
	 */
	FORCEINLINE void InitializeValue_InContainer( void* Dest ) const
	{
		if (PropertyFlags & CPF_ZeroConstructor)
		{
			FMemory::Memzero(ContainerPtrToValuePtr<void>(Dest),ElementSize * ArrayDim);
		}
		else
		{
			InitializeValueInternal(ContainerPtrToValuePtr<void>(Dest));
		}
	}
protected:
	virtual void InitializeValueInternal( void* Dest ) const;
public:

	/**
	 * Verify that modifying this property's value via ImportText is allowed.
	 * 
	 * @param	PortFlags	the flags specified in the call to ImportText
	 * @param	ErrorText	[out] set to the error message that should be displayed if returns false
	 *
	 * @return	true if ImportText should be allowed
	 */
	bool ValidateImportFlags( uint32 PortFlags, FOutputDevice* ErrorText = NULL ) const;
	bool ShouldPort( uint32 PortFlags=0 ) const;
	virtual FName GetID() const;

	/**
	 * Creates new copies of components
	 * 
	 * @param	Data				pointer to the address of the instanced object referenced by this UComponentProperty
	 * @param	DefaultData			pointer to the address of the default value of the instanced object referenced by this UComponentProperty
	 * @param	Owner				the object that contains this property's data
	 * @param	InstanceGraph		contains the mappings of instanced objects and components to their templates
	 */
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph );

	virtual int32 GetMinAlignment() const;

	/**
	 * Returns true if this property, or in the case of e.g. array or struct properties any sub- property, contains a
	 * UObject reference.
	 * @param	EncounteredStructProps		used to check for recursion in arrays
	 * @param	InReferenceType				type of object reference (strong / weak)
	 *
	 * @return true if property (or sub- properties) contains the specified type of UObject reference, false otherwise
	 */
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const;

	/**
	 * Returns true if this property, or in the case of e.g. array or struct properties any sub- property, contains a
	 * weak UObject reference.
	 *
	 * @return true if property (or sub- properties) contain a weak UObject reference, false otherwise
	 */
	bool ContainsWeakObjectReference() const
	{
		TArray<const FStructProperty*> EncounteredStructProps;
		return ContainsObjectReference(EncounteredStructProps, EPropertyObjectReferenceType::Weak);
	}

	/**
	 * Returns true if this property, or in the case of e.g. array or struct properties any sub- property, contains a
	 * UObject reference that is marked CPF_NeedCtorLink (i.e. instanced keyword).
	 *
	 * @return true if property (or sub- properties) contain a FObjectProperty that is marked CPF_NeedCtorLink, false otherwise
	 */
	FORCEINLINE bool ContainsInstancedObjectProperty() const
	{
		return (PropertyFlags&(CPF_ContainsInstancedReference | CPF_InstancedReference)) != 0;
	}

	/**
	 * Emits tokens used by realtime garbage collection code to passed in ReferenceTokenStream. The offset emitted is relative
	 * to the passed in BaseOffset which is used by e.g. arrays of structs.
	 */
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper);

	FORCEINLINE int32 GetSize() const
	{
		return ArrayDim * ElementSize;
	}
	bool ShouldSerializeValue( FArchive& Ar ) const;

	/**
	 * Determines whether this property value is eligible for copying when duplicating an object
	 * 
	 * @return	true if this property value should be copied into the duplicate object
	 */
	bool ShouldDuplicateValue() const
	{
		return ShouldPort() && GetOwnerClass() != UObject::StaticClass();
	}

	/**
	 * Returns the first FProperty in this property's Outer chain that does not have a FProperty for an Outer
	 */
	FProperty* GetOwnerProperty()
	{
		FProperty* Result = this;
		for (FProperty* PropBase = GetOwner<FProperty>(); PropBase; PropBase = PropBase->GetOwner<FProperty>())
		{
			Result = PropBase;
		}
		return Result;
	}

	const FProperty* GetOwnerProperty() const
	{
		const FProperty* Result = this;
		for (const FProperty* PropBase = GetOwner<FProperty>(); PropBase; PropBase = PropBase->GetOwner<FProperty>())
		{
			Result = PropBase;
		}
		return Result;
	}

	/**
	 * Returns this property's propertyflags
	 */
	FORCEINLINE EPropertyFlags GetPropertyFlags() const
	{
		return PropertyFlags;
	}
	FORCEINLINE void SetPropertyFlags( EPropertyFlags NewFlags )
	{
		PropertyFlags |= NewFlags;
	}
	FORCEINLINE void ClearPropertyFlags( EPropertyFlags NewFlags )
	{
		PropertyFlags &= ~NewFlags;
	}
	/**
	 * Used to safely check whether any of the passed in flags are set. This is required
	 * as PropertyFlags currently is a 64 bit data type and bool is a 32 bit data type so
	 * simply using PropertyFlags&CPF_MyFlagBiggerThanMaxInt won't work correctly when
	 * assigned directly to an bool.
	 *
	 * @param FlagsToCheck	Object flags to check for.
	 *
	 * @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	 */
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
	 * Returns the replication owner, which is the property itself, or NULL if this isn't important for replication.
	 * It is relevant if the property is a net relevant and not being run in the editor
	 */
	FORCEINLINE FProperty* GetRepOwner()
	{
		return (!GIsEditor && ((PropertyFlags & CPF_Net) != 0)) ? this : NULL;
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

	/** returns true, if Other is property of exactly the same type */
	virtual bool SameType(const FProperty* Other) const;

	ELifetimeCondition GetBlueprintReplicationCondition() const { return BlueprintReplicationCondition; }
	void SetBlueprintReplicationCondition(ELifetimeCondition InBlueprintReplicationCondition) { BlueprintReplicationCondition = InBlueprintReplicationCondition; }

#if HACK_HEADER_GENERATOR
	// Required by UHT makefiles for internal data serialization.
	friend struct FPropertyArchiveProxy;
#endif // HACK_HEADER_GENERATOR
};


class COREUOBJECT_API FPropertyHelpers
{
public:
	static const TCHAR* ReadToken( const TCHAR* Buffer, FString& Out, bool DottedNames = false);

	// @param Out Appended to
	static const TCHAR* ReadToken( const TCHAR* Buffer, FStringBuilderBase& Out, bool DottedNames = false);
};

namespace UEProperty_Private
{
	/** FProperty methods FOR INTERNAL USE ONLY -- only authorized users should be making use of this. -- DO NOT USE! */
	class FProperty_DoNotUse
	{
	public:
		/** 
		 * To facilitate runtime binding with native C++ data-members, we need 
		 * a way of updating a property's generated offset.
		 * This is needed for pre-generated properties, which are then loaded 
		 * later, and fixed up to point at explicitly mapped C++ data-members.
		 * 
		 * Explicitly exposed for this singular case -- DO NOT USE otherwise.
		 */
		static COREUOBJECT_API void Unsafe_AlterOffset(FProperty& Property, const int32 OffsetOverride)
		{
			Property.SetOffset_Internal(OffsetOverride);
		}
	};
}

/** reference to a property and optional array index used in property text import to detect duplicate references */
struct COREUOBJECT_API FDefinedProperty
{
    FProperty* Property;
    int32 Index;
    bool operator== (const FDefinedProperty& Other) const
    {
        return (Property == Other.Property && Index == Other.Index);
    }
};

/**
 * Creates a temporary object that represents the default constructed value of a FProperty
 */
class COREUOBJECT_API FDefaultConstructedPropertyElement
{
public:
	explicit FDefaultConstructedPropertyElement(FProperty* InProp)
		: Prop(InProp)
		, Obj(FMemory::Malloc(InProp->GetSize(), InProp->GetMinAlignment()))
	{
		InProp->InitializeValue(Obj);
	}

	~FDefaultConstructedPropertyElement()
	{
		Prop->DestroyValue(Obj);
		FMemory::Free(Obj);
	}

	void* GetObjAddress() const
	{
		return Obj;
	}

private:
	// Non-copyable
	FDefaultConstructedPropertyElement(const FDefaultConstructedPropertyElement&) = delete;
	FDefaultConstructedPropertyElement& operator=(const FDefaultConstructedPropertyElement&) = delete;

private:
	FProperty* Prop;
	void* Obj;
};


/*-----------------------------------------------------------------------------
	TProperty.
-----------------------------------------------------------------------------*/


template<typename InTCppType>
class TPropertyTypeFundamentals
{
public:
	/** Type of the CPP property **/
	typedef InTCppType TCppType;
	enum
	{
		CPPSize = sizeof(TCppType),
		CPPAlignment = alignof(TCppType)
	};

	static FORCEINLINE TCHAR const* GetTypeName()
	{
		return TNameOf<TCppType>::GetName();
	}

	/** Convert the address of a value of the property to the proper type */
	static FORCEINLINE TCppType const* GetPropertyValuePtr(void const* A)
	{
		return (TCppType const*)A;
	}
	/** Convert the address of a value of the property to the proper type */
	static FORCEINLINE TCppType* GetPropertyValuePtr(void* A)
	{
		return (TCppType*)A;
	}
	/** Get the value of the property from an address */
	static FORCEINLINE TCppType const& GetPropertyValue(void const* A)
	{
		return *GetPropertyValuePtr(A);
	}
	/** Get the default value of the cpp type, just the default constructor, which works even for things like in32 */
	static FORCEINLINE TCppType GetDefaultPropertyValue()
	{
		return TCppType();
	}
	/** Get the value of the property from an address, unless it is NULL, then return the default value */
	static FORCEINLINE TCppType GetOptionalPropertyValue(void const* B)
	{
		return B ? GetPropertyValue(B) : GetDefaultPropertyValue();
	}
	/** Set the value of a property at an address */
	static FORCEINLINE void SetPropertyValue(void* A, TCppType const& Value)
	{
		*GetPropertyValuePtr(A) = Value;
	}
	/** Initialize the value of a property at an address, this assumes over uninitialized memory */
	static FORCEINLINE TCppType* InitializePropertyValue(void* A)
	{
		return new (A) TCppType();
	}
	/** Destroy the value of a property at an address */
	static FORCEINLINE void DestroyPropertyValue(void* A)
	{
		GetPropertyValuePtr(A)->~TCppType();
	}

protected:
	/** Get the property flags corresponding to this C++ type, from the C++ type traits system */
	static FORCEINLINE EPropertyFlags GetComputedFlagsPropertyFlags()
	{
		return 
			(TIsPODType<TCppType>::Value ? CPF_IsPlainOldData : CPF_None) 
			| (TIsTriviallyDestructible<TCppType>::Value ? CPF_NoDestructor : CPF_None) 
			| (TIsZeroConstructType<TCppType>::Value ? CPF_ZeroConstructor : CPF_None)
			| (TModels<CGetTypeHashable, TCppType>::Value ? CPF_HasGetValueTypeHash : CPF_None);

	}
};


template<typename InTCppType, class TInPropertyBaseClass>
class TProperty : public TInPropertyBaseClass, public TPropertyTypeFundamentals<InTCppType>
{
public:

	typedef InTCppType TCppType;
	typedef TInPropertyBaseClass Super;
	typedef TPropertyTypeFundamentals<InTCppType> TTypeFundamentals;

	TProperty(EInternal InInernal, FFieldClass* InClass)
		: Super(EC_InternalUseOnlyConstructor, InClass)
	{
	}

	TProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
	{
		SetElementSize();
	}

	TProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags | TTypeFundamentals::GetComputedFlagsPropertyFlags())
	{
		SetElementSize();
	}

#if WITH_EDITORONLY_DATA
	explicit TProperty(UField* InField)
		: Super(InField)
	{
		SetElementSize();
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPType( FString* ExtendedTypeText=NULL, uint32 CPPExportFlags=0 ) const override
	{
		return FString(TTypeFundamentals::GetTypeName());
	}
	virtual bool PassCPPArgsByRef() const override
	{
		// non-pod data is passed by reference
		return !TIsPODType<TCppType>::Value;
	}
	// End of UHT interface

	// FProperty interface.
	virtual int32 GetMinAlignment() const override
	{
		return TTypeFundamentals::CPPAlignment;
	}
	virtual void LinkInternal(FArchive& Ar) override
	{
		SetElementSize();
		this->PropertyFlags |= TTypeFundamentals::GetComputedFlagsPropertyFlags();

	}
	virtual void CopyValuesInternal( void* Dest, void const* Src, int32 Count ) const override
	{
		for (int32 Index = 0; Index < Count; Index++)
		{
			TTypeFundamentals::GetPropertyValuePtr(Dest)[Index] = TTypeFundamentals::GetPropertyValuePtr(Src)[Index];
		}
	}
	virtual void ClearValueInternal( void* Data ) const override
	{
		TTypeFundamentals::SetPropertyValue(Data, TTypeFundamentals::GetDefaultPropertyValue());
	}
	virtual void InitializeValueInternal( void* Dest ) const override
	{
		for (int32 i = 0; i < this->ArrayDim; ++i)
		{
			TTypeFundamentals::InitializePropertyValue((uint8*)Dest + i * this->ElementSize);
		}
	}
	virtual void DestroyValueInternal( void* Dest ) const override
	{
		for (int32 i = 0; i < this->ArrayDim; ++i)
		{
			TTypeFundamentals::DestroyPropertyValue((uint8*)Dest + i * this->ElementSize);
		}
	}

	/** Convert the address of a container to the address of the property value, in the proper type */
	FORCEINLINE TCppType const* GetPropertyValuePtr_InContainer(void const* A, int32 ArrayIndex = 0) const
	{
		return TTypeFundamentals::GetPropertyValuePtr(Super::template ContainerPtrToValuePtr<void>(A, ArrayIndex));
	}
	/** Convert the address of a container to the address of the property value, in the proper type */
	FORCEINLINE TCppType* GetPropertyValuePtr_InContainer(void* A, int32 ArrayIndex = 0) const
	{
		return TTypeFundamentals::GetPropertyValuePtr(Super::template ContainerPtrToValuePtr<void>(A, ArrayIndex));
	}
	/** Get the value of the property from a container address */
	FORCEINLINE TCppType const& GetPropertyValue_InContainer(void const* A, int32 ArrayIndex = 0) const
	{
		return *GetPropertyValuePtr_InContainer(A, ArrayIndex);
	}
	/** Get the value of the property from a container address, unless it is NULL, then return the default value */
	FORCEINLINE TCppType GetOptionalPropertyValue_InContainer(void const* B, int32 ArrayIndex = 0) const
	{
		return B ? GetPropertyValue_InContainer(B, ArrayIndex) : TTypeFundamentals::GetDefaultPropertyValue();
	}
	/** Set the value of a property in a container */
	FORCEINLINE void SetPropertyValue_InContainer(void* A, TCppType const& Value, int32 ArrayIndex = 0) const
	{
		*GetPropertyValuePtr_InContainer(A, ArrayIndex) = Value;
	}

protected:
	FORCEINLINE void SetElementSize()
	{
		this->ElementSize = TTypeFundamentals::CPPSize;
	}
	// End of FProperty interface
};

template<typename InTCppType, class TInPropertyBaseClass>
class COREUOBJECT_API TProperty_WithEqualityAndSerializer : public TProperty<InTCppType, TInPropertyBaseClass>
{

public:
	typedef TProperty<InTCppType, TInPropertyBaseClass> Super;
	typedef InTCppType TCppType;
	typedef typename Super::TTypeFundamentals TTypeFundamentals;

	TProperty_WithEqualityAndSerializer(EInternal InInernal, FFieldClass* InClass)
		: Super(EC_InternalUseOnlyConstructor, InClass)
	{
	}

	TProperty_WithEqualityAndSerializer(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
	{
	}

	TProperty_WithEqualityAndSerializer(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit TProperty_WithEqualityAndSerializer(UField* InField)
		: Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	/** DO NOT USE. This constructor is for internal usage only for hot-reload purposes. */
	TProperty_WithEqualityAndSerializer(FVTableHelper& Helper) : Super(Helper) {};

	// FProperty interface.
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags=0 ) const override
	{
		// RHS is the same as TTypeFundamentals::GetOptionalPropertyValue(B) but avoids an unnecessary copy of B
		return TTypeFundamentals::GetPropertyValue(A) == (B ? TTypeFundamentals::GetPropertyValue(B) : TTypeFundamentals::GetDefaultPropertyValue());
	}
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override
	{
		Slot << *TTypeFundamentals::GetPropertyValuePtr(Value);
	}
	// End of FProperty interface

};

class COREUOBJECT_API FNumericProperty : public FProperty
{
	DECLARE_FIELD(FNumericProperty, FProperty, CASTCLASS_FNumericProperty)

	FNumericProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FProperty(InOwner, InName, InObjectFlags)
	{}

	FNumericProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: FProperty(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{}

#if WITH_EDITORONLY_DATA
	explicit FNumericProperty(UField* InField)
		: Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface.
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const override;

	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	// End of FProperty interface

	// FNumericProperty interface.

	/** Return true if this property is for a floating point number **/
	virtual bool IsFloatingPoint() const;

	/** Return true if this property is for a integral or enum type **/
	virtual bool IsInteger() const;

	template <typename T>
	bool CanHoldValue(T Value) const
	{
		if (!TIsFloatingPoint<T>::Value)
		{
			//@TODO: FLOATPRECISION: This feels wrong, it might be losing precision before it tests to see if it's going to lose precision...
			return CanHoldDoubleValueInternal((double)Value);
		}
		else if (TIsSigned<T>::Value)
		{
			return CanHoldSignedValueInternal(Value);
		}
		else
		{
			return CanHoldUnsignedValueInternal(Value);
		}
	}

	/** Return true if this property is a FByteProperty with a non-null Enum **/
	FORCEINLINE bool IsEnum() const
	{
		return !!GetIntPropertyEnum();
	}

	/** Return the UEnum if this property is a FByteProperty with a non-null Enum **/
	virtual UEnum* GetIntPropertyEnum() const;

	/** 
	 * Set the value of an unsigned integral property type
	 * @param Data - pointer to property data to set
	 * @param Value - Value to set data to
	**/
	virtual void SetIntPropertyValue(void* Data, uint64 Value) const;

	/** 
	 * Set the value of a signed integral property type
	 * @param Data - pointer to property data to set
	 * @param Value - Value to set data to
	**/
	virtual void SetIntPropertyValue(void* Data, int64 Value) const;

	/** 
	 * Set the value of a floating point property type
	 * @param Data - pointer to property data to set
	 * @param Value - Value to set data to
	**/
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const;

	/** 
	 * Set the value of any numeric type from a string point
	 * @param Data - pointer to property data to set
	 * @param Value - Value (as a string) to set 
	 * CAUTION: This routine does not do enum name conversion
	**/
	virtual void SetNumericPropertyValueFromString(void* Data, TCHAR const* Value) const;

	/** 
	 * Gets the value of a signed integral property type
	 * @param Data - pointer to property data to get
	 * @return Data as a signed int
	**/
	virtual int64 GetSignedIntPropertyValue(void const* Data) const;

	/** 
	 * Gets the value of an unsigned integral property type
	 * @param Data - pointer to property data to get
	 * @return Data as an unsigned int
	**/
	virtual uint64 GetUnsignedIntPropertyValue(void const* Data) const;

	/** 
	 * Gets the value of an floating point property type
	 * @param Data - pointer to property data to get
	 * @return Data as a double
	**/
	virtual double GetFloatingPointPropertyValue(void const* Data) const;

	/** 
	 * Get the value of any numeric type and return it as a string
	 * @param Data - pointer to property data to get
	 * @return Data as a string
	 * CAUTION: This routine does not do enum name conversion
	**/
	virtual FString GetNumericPropertyValueToString(void const* Data) const;
	// End of FNumericProperty interface

	static int64 ReadEnumAsInt64(FStructuredArchive::FSlot Slot, UStruct* DefaultsStruct, const FPropertyTag& Tag);

private:
	virtual bool CanHoldDoubleValueInternal  (double Value) const PURE_VIRTUAL(FNumericProperty::CanHoldDoubleValueInternal,   return false;);
	virtual bool CanHoldSignedValueInternal  (int64  Value) const PURE_VIRTUAL(FNumericProperty::CanHoldSignedValueInternal,   return false;);
	virtual bool CanHoldUnsignedValueInternal(uint64 Value) const PURE_VIRTUAL(FNumericProperty::CanHoldUnsignedValueInternal, return false;);
};

template<typename InTCppType>
class TProperty_Numeric : public TProperty_WithEqualityAndSerializer<InTCppType, FNumericProperty>
{
public:
	typedef TProperty_WithEqualityAndSerializer<InTCppType, FNumericProperty> Super;
	typedef InTCppType TCppType;
	typedef typename Super::TTypeFundamentals TTypeFundamentals;

	TProperty_Numeric(EInternal InInernal, FFieldClass* InClass)
		: Super(EC_InternalUseOnlyConstructor, InClass)
	{
	}

	TProperty_Numeric(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
	{
	}

	TProperty_Numeric(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit TProperty_Numeric(UField* InField)
		: Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	virtual FString GetCPPTypeForwardDeclaration() const override
	{
		return FString();
	}

	// FProperty interface
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override
	{
		return GetTypeHash(*(const InTCppType*)Src);
	}

protected:
	template <typename OldNumericType>
	FORCEINLINE void ConvertFromArithmeticValue(FStructuredArchive::FSlot Slot, void* Obj, const FPropertyTag& Tag) const
	{
		TConvertAndSet<OldNumericType, TCppType>(*this, Slot, Obj, Tag);
	}

private:
	template <typename FromType, typename ToType>
	struct TConvertAndSet
	{
		TConvertAndSet(const TProperty_Numeric& Property, FStructuredArchive::FSlot Slot, void* Obj, const FPropertyTag& Tag)
		{
			FromType OldValue;
			Slot << OldValue;
			ToType NewValue = (ToType)OldValue;
			Property.SetPropertyValue_InContainer(Obj, NewValue, Tag.ArrayIndex);

			UE_CLOG(
				((TIsSigned<FromType>::Value || TIsFloatingPoint<FromType>::Value) && (!TIsSigned<ToType>::Value && !TIsFloatingPoint<ToType>::Value) && OldValue < 0) || ((FromType)NewValue != OldValue),
				LogClass,
				Warning,
				TEXT("Potential data loss during conversion of integer property %s of %s - was (%s) now (%s) - for package: %s"),
				*Property.GetName(),
				*Slot.GetUnderlyingArchive().GetArchiveName(),
				*LexToString(OldValue),
				*LexToString(NewValue),
				*Slot.GetUnderlyingArchive().GetArchiveName()
			);
		}
	};

	template <typename SameType>
	struct TConvertAndSet<SameType, SameType>
	{
		FORCEINLINE TConvertAndSet(const TProperty_Numeric& Property, FStructuredArchive::FSlot Slot, void* Obj, const FPropertyTag& Tag)
		{
			SameType Value;
			Slot << Value;
			Property.SetPropertyValue_InContainer(Obj, Value, Tag.ArrayIndex);
		}
	};

public:
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override
	{
		if (const EName * TagType = Tag.Type.ToEName())
		{
			switch (*TagType)
			{
			case NAME_Int8Property:
				ConvertFromArithmeticValue<int8>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_Int16Property:
				ConvertFromArithmeticValue<int16>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_IntProperty:
				ConvertFromArithmeticValue<int32>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_Int64Property:
				ConvertFromArithmeticValue<int64>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_ByteProperty:
				if (!Tag.EnumName.IsNone())
				{
					int64 PreviousValue = this->ReadEnumAsInt64(Slot, DefaultsStruct, Tag);
					this->SetPropertyValue_InContainer(Data, (TCppType)PreviousValue, Tag.ArrayIndex);
				}
				else
				{
					ConvertFromArithmeticValue<int8>(Slot, Data, Tag);
				}
				return EConvertFromTypeResult::Converted;

			case NAME_EnumProperty:
			{
				int64 PreviousValue = this->ReadEnumAsInt64(Slot, DefaultsStruct, Tag);
				this->SetPropertyValue_InContainer(Data, (TCppType)PreviousValue, Tag.ArrayIndex);
				return EConvertFromTypeResult::Converted;
			}

			case NAME_UInt16Property:
				ConvertFromArithmeticValue<uint16>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_UInt32Property:
				ConvertFromArithmeticValue<uint32>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_UInt64Property:
				ConvertFromArithmeticValue<uint64>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_FloatProperty:
				ConvertFromArithmeticValue<float>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			case NAME_DoubleProperty:
				ConvertFromArithmeticValue<double>(Slot, Data, Tag);
				return EConvertFromTypeResult::Converted;

			default:
				// We didn't convert it
				break;
			}
		}

		return EConvertFromTypeResult::UseSerializeItem;
	}
	// End of FProperty interface

	// FNumericProperty interface.
	virtual bool IsFloatingPoint() const override
	{
		return TIsFloatingPoint<TCppType>::Value;
	}
	virtual bool IsInteger() const override
	{
		return TIsIntegral<TCppType>::Value;
	}
	virtual void SetIntPropertyValue(void* Data, uint64 Value) const override
	{
		check(TIsIntegral<TCppType>::Value);
		TTypeFundamentals::SetPropertyValue(Data, (TCppType)Value);
	}
	virtual void SetIntPropertyValue(void* Data, int64 Value) const override
	{
		check(TIsIntegral<TCppType>::Value);
		TTypeFundamentals::SetPropertyValue(Data, (TCppType)Value);
	}
	virtual void SetFloatingPointPropertyValue(void* Data, double Value) const override
	{
		check(TIsFloatingPoint<TCppType>::Value);
		TTypeFundamentals::SetPropertyValue(Data, (TCppType)Value);
	}
	virtual void SetNumericPropertyValueFromString(void* Data, TCHAR const* Value) const override
	{
		LexFromString(*TTypeFundamentals::GetPropertyValuePtr(Data), Value);
	}
	virtual FString GetNumericPropertyValueToString(void const* Data) const override
	{
		return LexToString(TTypeFundamentals::GetPropertyValue(Data));
	}
	virtual int64 GetSignedIntPropertyValue(void const* Data) const override
	{
		check(TIsIntegral<TCppType>::Value);
		return (int64)TTypeFundamentals::GetPropertyValue(Data);
	}
	virtual uint64 GetUnsignedIntPropertyValue(void const* Data) const override
	{
		check(TIsIntegral<TCppType>::Value);
		return (uint64)TTypeFundamentals::GetPropertyValue(Data);
	}
	virtual double GetFloatingPointPropertyValue(void const* Data) const override
	{
		check(TIsFloatingPoint<TCppType>::Value);
		return (double)TTypeFundamentals::GetPropertyValue(Data);
	}
	// End of FNumericProperty interface

private:
	virtual bool CanHoldDoubleValueInternal(double Value) const
	{
		return (double)(InTCppType)Value == Value;
	}

	virtual bool CanHoldSignedValueInternal(int64 Value) const
	{
		return (int64)(InTCppType)Value == Value;
	}

	virtual bool CanHoldUnsignedValueInternal(uint64 Value) const
	{
		return (uint64)(InTCppType)Value == Value;
	}
};

/*-----------------------------------------------------------------------------
	FByteProperty.
-----------------------------------------------------------------------------*/

//
// Describes an unsigned byte value or 255-value enumeration variable.
//
class COREUOBJECT_API FByteProperty : public TProperty_Numeric<uint8>
{
	DECLARE_FIELD(FByteProperty, TProperty_Numeric<uint8>, CASTCLASS_FByteProperty)

	// Variables.
	UEnum* Enum;

	FByteProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
		, Enum(nullptr)
	{
	}

	FByteProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UEnum* InEnum = nullptr)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	,	Enum( InEnum )
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FByteProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface.
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// UHT interface
	virtual FString GetCPPType( FString* ExtendedTypeText=NULL, uint32 CPPExportFlags=0 ) const override;
	// End of UHT interface

	// FProperty interface.
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	// FNumericProperty interface.
	virtual UEnum* GetIntPropertyEnum() const override;
	// End of FNumericProperty interface

	// Returns the number of bits required by NetSerializeItem to encode this property (may be fewer than 8 if this byte represents an enum)
	uint64 GetMaxNetSerializeBits() const;
};

/*-----------------------------------------------------------------------------
	FInt8Property.
-----------------------------------------------------------------------------*/

//
// Describes a 8-bit signed integer variable.
//
class COREUOBJECT_API FInt8Property : public TProperty_Numeric<int8>
{
	DECLARE_FIELD(FInt8Property, TProperty_Numeric<int8>, CASTCLASS_FInt8Property)

	FInt8Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FInt8Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FInt8Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

/*-----------------------------------------------------------------------------
	FInt16Property.
-----------------------------------------------------------------------------*/

//
// Describes a 16-bit signed integer variable.
//
class COREUOBJECT_API FInt16Property : public TProperty_Numeric<int16>
{
	DECLARE_FIELD(FInt16Property, TProperty_Numeric<int16>, CASTCLASS_FInt16Property)

	FInt16Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FInt16Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FInt16Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};


/*-----------------------------------------------------------------------------
	FIntProperty.
-----------------------------------------------------------------------------*/

//
// Describes a 32-bit signed integer variable.
//
class COREUOBJECT_API FIntProperty : public TProperty_Numeric<int32>
{
	DECLARE_FIELD(FIntProperty, TProperty_Numeric<int32>, CASTCLASS_FIntProperty)

	FIntProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FIntProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FIntProperty(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

/*-----------------------------------------------------------------------------
	FInt64Property.
-----------------------------------------------------------------------------*/

//
// Describes a 64-bit signed integer variable.
//
class COREUOBJECT_API FInt64Property : public TProperty_Numeric<int64>
{
	DECLARE_FIELD(FInt64Property, TProperty_Numeric<int64>, CASTCLASS_FInt64Property)

	FInt64Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FInt64Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FInt64Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

/*-----------------------------------------------------------------------------
	FUInt16Property.
-----------------------------------------------------------------------------*/

//
// Describes a 16-bit unsigned integer variable.
//
class COREUOBJECT_API FUInt16Property : public TProperty_Numeric<uint16>
{
	DECLARE_FIELD(FUInt16Property, TProperty_Numeric<uint16>, CASTCLASS_FUInt16Property)

	FUInt16Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FUInt16Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FUInt16Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

/*-----------------------------------------------------------------------------
	FUInt32Property.
-----------------------------------------------------------------------------*/

//
// Describes a 32-bit unsigned integer variable.
//
class COREUOBJECT_API FUInt32Property : public TProperty_Numeric<uint32>
{
	DECLARE_FIELD(FUInt32Property, TProperty_Numeric<uint32>, CASTCLASS_FUInt32Property)

	FUInt32Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FUInt32Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
	:	TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FUInt32Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

/*-----------------------------------------------------------------------------
	FUInt64Property.
-----------------------------------------------------------------------------*/

//
// Describes a 64-bit unsigned integer variable.
//
class COREUOBJECT_API FUInt64Property : public TProperty_Numeric<uint64>
{
	DECLARE_FIELD(FUInt64Property, TProperty_Numeric<uint64>, CASTCLASS_FUInt64Property)

	FUInt64Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FUInt64Property(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FUInt64Property(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};


/*-----------------------------------------------------------------------------
	Aliases for implicitly-sized integer properties.
-----------------------------------------------------------------------------*/

namespace UETypes_Private
{
	template <typename IntType> struct TIntegerPropertyMapping;

	template <> struct TIntegerPropertyMapping<int8>   { typedef FInt8Property   Type; };
	template <> struct TIntegerPropertyMapping<int16>  { typedef FInt16Property  Type; };
	template <> struct TIntegerPropertyMapping<int32>  { typedef FIntProperty    Type; };
	template <> struct TIntegerPropertyMapping<int64>  { typedef FInt64Property  Type; };
	template <> struct TIntegerPropertyMapping<uint8>  { typedef FByteProperty   Type; };
	template <> struct TIntegerPropertyMapping<uint16> { typedef FUInt16Property Type; };
	template <> struct TIntegerPropertyMapping<uint32> { typedef FUInt32Property Type; };
	template <> struct TIntegerPropertyMapping<uint64> { typedef FUInt64Property Type; };
}

typedef UETypes_Private::TIntegerPropertyMapping<signed int>::Type UUnsizedIntProperty;
typedef UETypes_Private::TIntegerPropertyMapping<unsigned int>::Type UUnsizedFIntProperty;


/*-----------------------------------------------------------------------------
	FFloatProperty.
-----------------------------------------------------------------------------*/

//
// Describes an IEEE 32-bit floating point variable.
//
class COREUOBJECT_API FFloatProperty : public TProperty_Numeric<float>
{
	DECLARE_FIELD(FFloatProperty, TProperty_Numeric<float>, CASTCLASS_FFloatProperty)

	FFloatProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FFloatProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FFloatProperty(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual void ExportTextItem(FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const override;
	// End of FProperty interface
};

/*-----------------------------------------------------------------------------
	FDoubleProperty.
-----------------------------------------------------------------------------*/

//
// Describes an IEEE 64-bit floating point variable.
//
class COREUOBJECT_API FDoubleProperty : public TProperty_Numeric<double>
{
	DECLARE_FIELD(FDoubleProperty, TProperty_Numeric<double>, CASTCLASS_FDoubleProperty)

	FDoubleProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}

	FDoubleProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FDoubleProperty(UField* InField)
		: TProperty_Numeric(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA
};

// Note: Stub only. Used to provide FNativeClassHeaderGenerator::PropertyNew with a way to track an as yet unaliased FFloatProperty/FDoubleProperty within the header tool, which will be resolved to the correct type at run time.
class COREUOBJECT_API FLargeWorldCoordinatesRealProperty : public TProperty_Numeric<double>
{
	DECLARE_FIELD(FLargeWorldCoordinatesRealProperty, TProperty_Numeric<double>, CASTCLASS_FLargeWorldCoordinatesRealProperty)

	FLargeWorldCoordinatesRealProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_Numeric(InOwner, InName, InObjectFlags)
	{
	}
};

/*-----------------------------------------------------------------------------
	FBoolProperty.
-----------------------------------------------------------------------------*/

//
// Describes a single bit flag variable residing in a 32-bit unsigned double word.
//
class COREUOBJECT_API FBoolProperty : public FProperty
{
	DECLARE_FIELD(FBoolProperty, FProperty, CASTCLASS_FBoolProperty)

	// Variables.
private:

	/** Size of the bitfield/bool property. Equal to ElementSize but used to check if the property has been properly initialized (0-8, where 0 means uninitialized). */
	uint8 FieldSize;
	/** Offset from the memeber variable to the byte of the property (0-7). */
	uint8 ByteOffset;
	/** Mask of the byte with the property value. */
	uint8 ByteMask;
	/** Mask of the field with the property value. Either equal to ByteMask or 255 in case of 'bool' type. */
	uint8 FieldMask;

public:

	FBoolProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);

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
	FBoolProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, uint32 InBitMask, uint32 InElementSize, bool bIsNativeBool);

#if WITH_EDITORONLY_DATA
	explicit FBoolProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface.
	virtual void Serialize( FArchive& Ar ) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// UHT interface
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// FProperty interface.
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem( FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const override;
	virtual void CopyValuesInternal( void* Dest, void const* Src, int32 Count ) const override;
	virtual void ClearValueInternal( void* Data ) const override;
	virtual void InitializeValueInternal( void* Dest ) const override;
	virtual int32 GetMinAlignment() const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	// Emulate the CPP type API, see TPropertyTypeFundamentals
	// this is incomplete as some operations make no sense for bitfields, for example they don't have a usable address
	typedef bool TCppType;
	FORCEINLINE bool GetPropertyValue(void const* A) const
	{
		check(FieldSize != 0);
		uint8* ByteValue = (uint8*)A + ByteOffset;
		return !!(*ByteValue & FieldMask);
	}
	FORCEINLINE bool GetPropertyValue_InContainer(void const* A, int32 ArrayIndex = 0) const
	{
		return GetPropertyValue(ContainerPtrToValuePtr<void>(A, ArrayIndex));
	}
	static FORCEINLINE bool GetDefaultPropertyValue()
	{
		return false;
	}
	FORCEINLINE bool GetOptionalPropertyValue(void const* B) const
	{
		return B ? GetPropertyValue(B) : GetDefaultPropertyValue();
	}
	FORCEINLINE bool GetOptionalPropertyValue_InContainer(void const* B, int32 ArrayIndex = 0) const
	{
		return B ? GetPropertyValue_InContainer(B, ArrayIndex) : GetDefaultPropertyValue();
	}
	FORCEINLINE void SetPropertyValue(void* A, bool Value) const
	{
		check(FieldSize != 0);
		uint8* ByteValue = (uint8*)A + ByteOffset;
		*ByteValue = ((*ByteValue) & ~FieldMask) | (Value ? ByteMask : 0);
	}
	FORCEINLINE void SetPropertyValue_InContainer(void* A, bool Value, int32 ArrayIndex = 0) const
	{
		SetPropertyValue(ContainerPtrToValuePtr<void>(A, ArrayIndex), Value);
	}
	// End of the CPP type API

	/** 
	 * Sets the bitfield/bool type and size. 
	 * This function must be called before FBoolProperty can be used.
	 *
	 * @param InSize size of the bitfield/bool type.
	 * @param bIsNativeBool true if this property represents C++ bool type.
	 */
	void SetBoolSize( const uint32 InSize, const bool bIsNativeBool = false, const uint32 InBitMask = 0 );

	/**
	 * If the return value is true this FBoolProperty represents C++ bool type.
	 */
	FORCEINLINE bool IsNativeBool() const
	{
		return FieldMask == 0xff;
	}

	uint32 GetValueTypeHashInternal(const void* Src) const override;

#if HACK_HEADER_GENERATOR
	// Required by UHT makefiles for internal data serialization.
	friend struct FBoolPropertyArchiveProxy;
#endif // HACK_HEADER_GENERATOR
};

/*-----------------------------------------------------------------------------
	FObjectPropertyBase.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class COREUOBJECT_API FObjectPropertyBase : public FProperty
{
	DECLARE_FIELD(FObjectPropertyBase, FProperty, CASTCLASS_FObjectPropertyBase)

public:

	// Variables.
	class UClass* PropertyClass;

	FObjectPropertyBase(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FProperty(InOwner, InName, InObjectFlags)
		, PropertyClass(nullptr)
	{}

	FObjectPropertyBase(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass = NULL)
		: FProperty(InOwner, InName, InObjectFlags, InOffset, InFlags)
		, PropertyClass(InClass)
	{}

#if WITH_EDITORONLY_DATA
	explicit FObjectPropertyBase(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void BeginDestroy() override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// FProperty interface
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual bool SupportsNetSharedSerialization() const override { return false; }
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual FName GetID() const override;
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph ) override;
	virtual bool SameType(const FProperty* Other) const override;
	/**
	 * Copy the value for a single element of this property. To the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET + INDEX * SIZE, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 *									INDEX = the index that you want to copy.  for properties which are not arrays, this should always be 0
	 *									SIZE = the ElementSize of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopySingleValueToScriptVM( void* Dest, void const* Src ) const override;

	/**
	 * Copy the value for all elements of this property. To the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopyCompleteValueToScriptVM( void* Dest, void const* Src ) const override;

	/**
	 * Copy the value for a single element of this property. From the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET + INDEX * SIZE, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 *									INDEX = the index that you want to copy.  for properties which are not arrays, this should always be 0
	 *									SIZE = the ElementSize of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopySingleValueFromScriptVM( void* Dest, void const* Src ) const override;

	/**
	 * Copy the value for all elements of this property. From the script VM.
	 * 
	 * @param	Dest				the address where the value should be copied to.  This should always correspond to the BASE + OFFSET, where
	 *									BASE = (for member properties) the address of the UObject which contains this data, (for locals/parameters) the address of the space allocated for the function's locals
	 *									OFFSET = the Offset of this FProperty
	 * @param	Src					the address of the value to copy from. should be evaluated the same way as Dest
	 */
	virtual void CopyCompleteValueFromScriptVM( void* Dest, void const* Src ) const override;
	// End of FProperty interface

	// FObjectPropertyBase interface
public:

	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName) const PURE_VIRTUAL(FObjectPropertyBase::GetCPPTypeCustom, return TEXT(""););

	/**
	 * Parses a text buffer into an object reference.
	 *
	 * @param	Property			the property that the value is being importing to
	 * @param	OwnerObject			the object that is importing the value; used for determining search scope.
	 * @param	RequiredMetaClass	the meta-class for the object to find; if the object that is resolved is not of this class type, the result is NULL.
	 * @param	PortFlags			bitmask of EPropertyPortFlags that can modify the behavior of the search
	 * @param	Buffer				the text to parse; should point to a textual representation of an object reference.  Can be just the object name (either fully 
	 *								fully qualified or not), or can be formatted as a const object reference (i.e. SomeClass'SomePackage.TheObject')
	 *								When the function returns, Buffer will be pointing to the first character after the object value text in the input stream.
	 * @param	ResolvedValue		receives the object that is resolved from the input text.
	 * @param InSerializeContext	Additional context when called during serialization
	 * @param bAllowAnyPackage		allows ignoring package name to find any object that happens to be loaded with the same name
	 * @return	true if the text is successfully resolved into a valid object reference of the correct type, false otherwise.
	 */
	static bool ParseObjectPropertyValue( const FProperty* Property, UObject* OwnerObject, UClass* RequiredMetaClass, uint32 PortFlags, const TCHAR*& Buffer, UObject*& out_ResolvedValue, FUObjectSerializeContext* InSerializeContext = nullptr, bool bAllowAnyPackage = true );
	static UObject* FindImportedObject( const FProperty* Property, UObject* OwnerObject, UClass* ObjectClass, UClass* RequiredMetaClass, const TCHAR* Text, uint32 PortFlags = 0, FUObjectSerializeContext* InSerializeContext = nullptr, bool bAllowAnyPackage = true );
	
	// Returns the qualified export path for a given object, parent, and export root scope
	static FString GetExportPath(const UObject* Object, const UObject* Parent, const UObject* ExportRootScope, const uint32 PortFlags);

	// Helper method for sharing code with FObjectPtrProperty even though one doesn't inherit from the other
	static bool StaticIdentical(UObject* A, UObject* B, uint32 PortFlags);

	virtual UObject* LoadObjectPropertyValue(const void* PropertyValueAddress) const
	{
		return GetObjectPropertyValue(PropertyValueAddress);
	}
	FORCEINLINE UObject* LoadObjectPropertyValue_InContainer(const void* PropertyValueAddress, int32 ArrayIndex = 0) const
	{
		return LoadObjectPropertyValue(ContainerPtrToValuePtr<void>(PropertyValueAddress, ArrayIndex));
	}
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const;
	FORCEINLINE UObject* GetObjectPropertyValue_InContainer(const void* PropertyValueAddress, int32 ArrayIndex = 0) const
	{
		return GetObjectPropertyValue(ContainerPtrToValuePtr<void>(PropertyValueAddress, ArrayIndex));
	}
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const;
	FORCEINLINE void SetObjectPropertyValue_InContainer(void* PropertyValueAddress, UObject* Value, int32 ArrayIndex = 0) const
	{
		SetObjectPropertyValue(ContainerPtrToValuePtr<void>(PropertyValueAddress, ArrayIndex), Value);
	}

	/**
	 * Setter function for this property's PropertyClass member. Favor this 
	 * function whilst loading (since, to handle circular dependencies, we defer 
	 * some class loads and use a placeholder class instead). It properly 
	 * handles deferred loading placeholder classes (so they can properly be 
	 * replaced later).
	 *  
	 * @param  NewPropertyClass    The PropertyClass you want this property set with.
	 */
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	void SetPropertyClass(UClass* NewPropertyClass);
#else
	FORCEINLINE void SetPropertyClass(UClass* NewPropertyClass) { PropertyClass = NewPropertyClass; }
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING

	virtual void CheckValidObject(void* Value) const;

protected:
	virtual bool AllowCrossLevel() const;
	virtual bool AllowObjectTypeReinterpretationTo(const FObjectPropertyBase* Other) const;
	// End of FObjectPropertyBase interface
};

template<typename InTCppType>
class COREUOBJECT_API TFObjectPropertyBase : public TProperty<InTCppType, FObjectPropertyBase>
{
public:
	typedef TProperty<InTCppType, FObjectPropertyBase> Super;
	typedef InTCppType TCppType;
	typedef typename Super::TTypeFundamentals TTypeFundamentals;

	TFObjectPropertyBase(EInternal InInernal, FFieldClass* InClass)
		: Super(EC_InternalUseOnlyConstructor, InClass)
	{
	}

	TFObjectPropertyBase(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
	{
		this->PropertyClass = nullptr;
	}


	TFObjectPropertyBase(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
		this->PropertyClass = InClass;
	}

#if WITH_EDITORONLY_DATA
	explicit TFObjectPropertyBase(UField* InField)
		: Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface.
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override
	{
		return (!!(InReferenceType & EPropertyObjectReferenceType::Strong) && !TIsWeakPointerType<InTCppType>::Value) ||
			(!!(InReferenceType & EPropertyObjectReferenceType::Weak) && TIsWeakPointerType<InTCppType>::Value);
	}
	// End of FProperty interface

	// TProperty::GetCPPType should not be used here
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override
	{
		check(this->PropertyClass);
		return this->GetCPPTypeCustom(ExtendedTypeText, CPPExportFlags, 
			FString::Printf(TEXT("%s%s"), this->PropertyClass->GetPrefixCPP(), *this->PropertyClass->GetName()));
	}
};


//
// Describes a reference variable to another object which may be nil.
//
class COREUOBJECT_API FObjectProperty : public TFObjectPropertyBase<UObject*>
{
	DECLARE_FIELD(FObjectProperty, TFObjectPropertyBase<UObject*>, CASTCLASS_FObjectProperty)

	FObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags)
	{
	}

	FObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags, InOffset, InFlags, InClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FObjectProperty(UField* InField)
		: TFObjectPropertyBase(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// FProperty interface
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual const TCHAR* ImportText_Internal(const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;

private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:
	// End of FProperty interface

	// FObjectPropertyBase interface
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName)  const override;
	// End of FObjectPropertyBase interface
};

//
// Describes a reference variable to another object which may be nil.
//
class COREUOBJECT_API FObjectPtrProperty : public FObjectProperty
{
	DECLARE_FIELD(FObjectPtrProperty, FObjectProperty, CASTCLASS_FObjectPtrProperty)

	using Super::Super;

#if WITH_EDITORONLY_DATA
	explicit FObjectPtrProperty(UField* InField)
		: FObjectProperty(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	// End of UHT interface

	// FProperty interface
	virtual bool SameType(const FProperty* Other) const override;
	virtual bool Identical(const void* A, const void* B, uint32 PortFlags) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	// End of FProperty interface

	// Helper method for sharing code with FClassPtrProperty even though one doesn't inherit from the other
	static void StaticSerializeItem(const FObjectPropertyBase* ObjectProperty, FStructuredArchive::FSlot Slot, void* Value, void const* Defaults);

	// FObjectProperty interface
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
	virtual bool AllowCrossLevel() const override;
	virtual bool AllowObjectTypeReinterpretationTo(const FObjectPropertyBase* Other) const override;
private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:
	// End of FObjectProperty interface
};

//
// Describes a reference variable to another object which may be nil, and may turn nil at any point
//
class COREUOBJECT_API FWeakObjectProperty : public TFObjectPropertyBase<FWeakObjectPtr>
{
	DECLARE_FIELD(FWeakObjectProperty, TFObjectPropertyBase<FWeakObjectPtr>, CASTCLASS_FWeakObjectProperty)

	FWeakObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags)
	{
	}

	FWeakObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags, InOffset, InFlags, InClass)
	{
	}
	
#if WITH_EDITORONLY_DATA
	explicit FWeakObjectProperty(UField* InField)
		: TFObjectPropertyBase(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// FProperty interface
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:
	// End of FProperty interface

	// FObjectProperty interface
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
	// End of FObjectProperty interface
};

//
// Describes a reference variable to another object which may be nil, and will become valid or invalid at any point
//
class COREUOBJECT_API FLazyObjectProperty : public TFObjectPropertyBase<FLazyObjectPtr>
{
	DECLARE_FIELD(FLazyObjectProperty, TFObjectPropertyBase<FLazyObjectPtr>, CASTCLASS_FLazyObjectProperty)

	FLazyObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags)
	{
	}

	FLazyObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags, InOffset, InFlags, InClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FLazyObjectProperty(UField* InField)
		: TFObjectPropertyBase(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;

	// End of UHT interface

	// FProperty interface
	virtual FName GetID() const override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem( FStructuredArchive::FSlot Slot, void* Value, void const* Defaults ) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	// End of FProperty interface

	// FObjectProperty interface
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
	virtual bool AllowCrossLevel() const override;
private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:
	// End of FObjectProperty interface
};

//
// Describes a reference variable to another object which may be nil, and will become valid or invalid at any point
//
class COREUOBJECT_API FSoftObjectProperty : public TFObjectPropertyBase<FSoftObjectPtr>
{
	DECLARE_FIELD(FSoftObjectProperty, TFObjectPropertyBase<FSoftObjectPtr>, CASTCLASS_FSoftObjectProperty)

	FSoftObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags)
	{}

	FSoftObjectProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InClass)
		: TFObjectPropertyBase(InOwner, InName, InObjectFlags, InOffset, InFlags, InClass)
	{}

#if WITH_EDITORONLY_DATA
	explicit FSoftObjectProperty(UField* InField)
		: TFObjectPropertyBase(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// FProperty interface
	virtual FName GetID() const override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem( FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	// End of FProperty interface

	// FObjectProperty interface
	virtual UObject* LoadObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
	virtual bool AllowCrossLevel() const override;
	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName)  const override;

	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override
	{
		if (ensureMsgf(PropertyClass, TEXT("Soft object property missing PropertyClass: %s"), *GetFullNameSafe(this)))
		{
			return Super::GetCPPType(ExtendedTypeText, CPPExportFlags);
		}
		else
		{
			return TEXT("TSoftObjectPtr<UObject>");
		}
	}

private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:

	// ScriptVM should store Asset as FSoftObjectPtr not as UObject.

	virtual void CopySingleValueToScriptVM(void* Dest, void const* Src) const override;
	virtual void CopyCompleteValueToScriptVM(void* Dest, void const* Src) const override;
	virtual void CopySingleValueFromScriptVM(void* Dest, void const* Src) const override;
	virtual void CopyCompleteValueFromScriptVM(void* Dest, void const* Src) const override;
	// End of FObjectProperty interface
};

/*-----------------------------------------------------------------------------
	FClassProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class COREUOBJECT_API FClassProperty : public FObjectProperty
{
	DECLARE_FIELD(FClassProperty, FObjectProperty, CASTCLASS_FClassProperty)

	// Variables.
	class UClass* MetaClass;
public:

	FClassProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FObjectProperty(InOwner, InName, InObjectFlags)
		, MetaClass(nullptr)
	{
	}

	FClassProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass, UClass* InClassType)
		: FObjectProperty(InOwner, InName, InObjectFlags, InOffset, InFlags, InClassType ? InClassType : UClass::StaticClass())
		, MetaClass(InMetaClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FClassProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void BeginDestroy() override;
	// End of UObject interface

	// Field Interface
	virtual void PostDuplicate(const FField& InField) override;

	// UHT interface
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags)  const override;
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// FProperty interface
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName)  const override;

	/**
	 * Setter function for this property's MetaClass member. Favor this function 
	 * whilst loading (since, to handle circular dependencies, we defer some 
	 * class loads and use a placeholder class instead). It properly handles 
	 * deferred loading placeholder classes (so they can properly be replaced 
	 * later).
	 * 
	 * @param  NewMetaClass    The MetaClass you want this property set with.
	 */
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	void SetMetaClass(UClass* NewMetaClass);
#else
	FORCEINLINE void SetMetaClass(UClass* NewMetaClass) { MetaClass = NewMetaClass; }
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
};

//
// Describes a reference variable to another object which may be nil.
//
class COREUOBJECT_API FClassPtrProperty : public FClassProperty
{
	DECLARE_FIELD(FClassPtrProperty, FClassProperty, CASTCLASS_FClassPtrProperty)

	using Super::Super;

#if WITH_EDITORONLY_DATA
	explicit FClassPtrProperty(UField* InField)
		: FClassProperty(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	// End of UHT interface

	// FProperty interface
	virtual bool SameType(const FProperty* Other) const override;
	virtual bool Identical(const void* A, const void* B, uint32 PortFlags) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	// End of FProperty interface

	// FObjectProperty interface
	virtual UObject* GetObjectPropertyValue(const void* PropertyValueAddress) const override;
	virtual void SetObjectPropertyValue(void* PropertyValueAddress, UObject* Value) const override;
private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const override;
public:
	// End of FObjectProperty interface
};

/*-----------------------------------------------------------------------------
	FSoftClassProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another class which may be nil, and will become valid or invalid at any point
//
class COREUOBJECT_API FSoftClassProperty : public FSoftObjectProperty
{
	DECLARE_FIELD(FSoftClassProperty, FSoftObjectProperty, CASTCLASS_FSoftClassProperty)

	// Variables.
	class UClass* MetaClass;
public:

	FSoftClassProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
		, MetaClass(nullptr)
	{}

	FSoftClassProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InMetaClass)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags, UClass::StaticClass())
		, MetaClass(InMetaClass)
	{}

#if WITH_EDITORONLY_DATA
	explicit FSoftClassProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override;
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// Field Interface
	virtual void PostDuplicate(const FField& InField) override;

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void BeginDestroy() override;
	// End of UObject interface

	// FProperty interface
	virtual bool SameType(const FProperty* Other) const override;
	// End of FProperty interface

	virtual FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerNativeTypeName)  const override;

	/**
	 * Setter function for this property's MetaClass member. Favor this function 
	 * whilst loading (since, to handle circular dependencies, we defer some 
	 * class loads and use a placeholder class instead). It properly handles 
	 * deferred loading placeholder classes (so they can properly be replaced 
	 * later).
	 * 
	 * @param  NewMetaClass    The MetaClass you want this property set with.
	 */
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	void SetMetaClass(UClass* NewMetaClass);
#else
	FORCEINLINE void SetMetaClass(UClass* NewMetaClass) { MetaClass = NewMetaClass; }
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
};

/*-----------------------------------------------------------------------------
	FInterfaceProperty.
-----------------------------------------------------------------------------*/

/**
 * This variable type provides safe access to a native interface pointer.  The data class for this variable is FScriptInterface, and is exported to auto-generated
 * script header files as a TScriptInterface.
 */

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FScriptInterface, FProperty> FInterfaceProperty_Super;

class COREUOBJECT_API FInterfaceProperty : public FInterfaceProperty_Super
{
	DECLARE_FIELD(FInterfaceProperty, FInterfaceProperty_Super, CASTCLASS_FInterfaceProperty)

	/** The native interface class that this interface property refers to */
	class	UClass*		InterfaceClass;
public:
	typedef FInterfaceProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FInterfaceProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FInterfaceProperty_Super(InOwner, InName, InObjectFlags)
		, InterfaceClass(nullptr)
	{
	}

	FInterfaceProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UClass* InInterfaceClass)
		: FInterfaceProperty_Super(InOwner, InName, InObjectFlags, InOffset, (InFlags & ~CPF_InterfaceClearMask))
		, InterfaceClass(InInterfaceClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FInterfaceProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UHT interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	// End of UHT interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// FProperty interface
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem( FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual bool SupportsNetSharedSerialization() const override { return false; }
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual bool SameType(const FProperty* Other) const override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual void BeginDestroy() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
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
#if USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
	void SetInterfaceClass(UClass* NewInterfaceClass);
#else
	FORCEINLINE void SetInterfaceClass(UClass* NewInterfaceClass) { InterfaceClass = NewInterfaceClass; }
#endif // USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING
};

/*-----------------------------------------------------------------------------
	FNameProperty.
-----------------------------------------------------------------------------*/

//
// Describes a name variable pointing into the global name table.
//

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty_WithEqualityAndSerializer<FName, FProperty> FNameProperty_Super;

class COREUOBJECT_API FNameProperty : public FNameProperty_Super
{
	DECLARE_FIELD(FNameProperty, FNameProperty_Super, CASTCLASS_FNameProperty)
public:
	typedef FNameProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FNameProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FNameProperty_Super(InOwner, InName, InObjectFlags)
	{
	}

	FNameProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: FNameProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FNameProperty(UField* InField)
		: FNameProperty_Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	uint32 GetValueTypeHashInternal(const void* Src) const override;
	// End of FProperty interface
};

/*-----------------------------------------------------------------------------
	FStrProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic string variable.
//

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty_WithEqualityAndSerializer<FString, FProperty> FStrProperty_Super;

class COREUOBJECT_API FStrProperty : public FStrProperty_Super
{
	DECLARE_FIELD(FStrProperty, FStrProperty_Super, CASTCLASS_FStrProperty)
public:
	typedef FStrProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FStrProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FStrProperty_Super(InOwner, InName, InObjectFlags)
	{
	}

	FStrProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags)
		: FStrProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FStrProperty(UField* InField)
		: FStrProperty_Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	uint32 GetValueTypeHashInternal(const void* Src) const override;
	// End of FProperty interface

	// Necessary to fix Compiler Error C2026
	static FString ExportCppHardcodedText(const FString& InSource, const FString& Indent);
};

/*-----------------------------------------------------------------------------
	FArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic array.
//

using FFreezableScriptArray = TScriptArray<TMemoryImageAllocator<DEFAULT_ALIGNMENT>>;

#if !PLATFORM_ANDROID || !PLATFORM_32BITS
	static_assert(sizeof(FScriptArray) == sizeof(FFreezableScriptArray) && alignof(FScriptArray) == alignof(FFreezableScriptArray), "FScriptArray and FFreezableScriptArray are expected to be layout-compatible");
#endif


// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FScriptArray, FProperty> FArrayProperty_Super;
class FScriptArrayHelper;

class COREUOBJECT_API FArrayProperty : public FArrayProperty_Super
{
	DECLARE_FIELD(FArrayProperty, FArrayProperty_Super, CASTCLASS_FArrayProperty)

	// Variables.
	FProperty* Inner;
	EArrayPropertyFlags ArrayFlags;

public:
	/** Type of the CPP property **/
	enum
	{
		// These need to be the same as FFreezableScriptArray
		CPPSize = sizeof(FScriptArray),
		CPPAlignment = alignof(FScriptArray)
	};

	typedef FArrayProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FArrayProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, EArrayPropertyFlags InArrayPropertyFlags=EArrayPropertyFlags::None)
		: FArrayProperty_Super(InOwner, InName, InObjectFlags)
		, Inner(nullptr)
	{
		ArrayFlags = InArrayPropertyFlags;
		SetElementSize();
	}

	FArrayProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, EArrayPropertyFlags InArrayPropertyFlags)
		: FArrayProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
		, Inner(nullptr)
	{
		ArrayFlags = InArrayPropertyFlags;
		SetElementSize();
	}

	virtual ~FArrayProperty();

#if WITH_EDITORONLY_DATA
	explicit FArrayProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// UField interface
	virtual void AddCppProperty(FProperty* Property) override;
	virtual FField* GetInnerFieldByName(const FName& InName) override;
	virtual void GetInnerFields(TArray<FField*>& OutFields) override;

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
	virtual void InitializeValueInternal(void* Dest) const override
	{
		if (EnumHasAnyFlags(ArrayFlags, EArrayPropertyFlags::UsesMemoryImageAllocator))
		{
			checkf(!PLATFORM_ANDROID || !PLATFORM_32BITS, TEXT("FFreezableScriptArray is not supported on Android 32 bit platform"));

			for (int32 i = 0; i < this->ArrayDim; ++i)
			{
				new ((uint8*)Dest + i * this->ElementSize) FFreezableScriptArray;
			}
		}
		else
		{
			for (int32 i = 0; i < this->ArrayDim; ++i)
			{
				new ((uint8*)Dest + i * this->ElementSize) FScriptArray;
			}
		}
	}
	virtual void CopyValuesInternal( void* Dest, void const* Src, int32 Count  ) const override;
	virtual void ClearValueInternal( void* Data ) const override;
	virtual void DestroyValueInternal( void* Dest ) const override;
	virtual bool PassCPPArgsByRef() const override;
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph ) override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;

	virtual int32 GetMinAlignment() const override
	{
		// This is the same as alignof(FFreezableScriptArray)
		return alignof(FScriptArray);
	}
	// End of FProperty interface

	FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& InnerTypeText, const FString& InInnerExtendedTypeText) const;

	/** Called by ExportTextItem, but can also be used by a non-ArrayProperty whose ArrayDim is > 1. */
	static void ExportTextInnerItem(FString& ValueStr, const FProperty* Inner, const void* PropertyValue, int32 PropertySize, const void* DefaultValue, int32 DefaultSize, UObject* Parent = nullptr, int32 PortFlags = 0, UObject* ExportRootScope = nullptr);

	/** Called by ImportTextItem, but can also be used by a non-ArrayProperty whose ArrayDim is > 1. ArrayHelper should be supplied by ArrayProperties and nullptr for fixed-size arrays. */
	static const TCHAR* ImportTextInnerItem(const TCHAR* Buffer, const FProperty* Inner, void* Data, int32 PortFlags, UObject* OwnerObject, FScriptArrayHelper* ArrayHelper = nullptr, FOutputDevice* ErrorText = (FOutputDevice*)GWarn);
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif

private:
	FORCEINLINE void SetElementSize()
	{
		this->ElementSize = CPPSize;
	}
};

using FFreezableScriptMap = TScriptMap<FMemoryImageSetAllocator>;

//@todo stever sizeof(FScriptMap) is 80 bytes, while sizeof(FFreezableScriptMap) is 56 bytes atm
//static_assert(sizeof(FScriptMap) == sizeof(FFreezableScriptMap) && alignof(FScriptMap) == alignof(FFreezableScriptMap), "FScriptMap and FFreezableScriptMap are expected to be layout-compatible");

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FScriptMap, FProperty> FMapProperty_Super;

class COREUOBJECT_API FMapProperty : public FMapProperty_Super
{
	DECLARE_FIELD(FMapProperty, FMapProperty_Super, CASTCLASS_FMapProperty)

	// Properties representing the key type and value type of the contained pairs
	FProperty*       KeyProp;
	FProperty*       ValueProp;
	FScriptMapLayout MapLayout;
	EMapPropertyFlags MapFlags;

	template <typename CallableType>
	auto WithScriptMap(void* InMap, CallableType&& Callable) const
	{
		if (!!(MapFlags & EMapPropertyFlags::UsesMemoryImageAllocator))
		{
			return Callable((FFreezableScriptMap*)InMap);
		}
		else
		{
			return Callable((FScriptMap*)InMap);
		}
	}

public:
	typedef FMapProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FMapProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, EMapPropertyFlags InMapFlags=EMapPropertyFlags::None);
	FMapProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, EMapPropertyFlags InMapFlags);

#if WITH_EDITORONLY_DATA
	explicit FMapProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	virtual ~FMapProperty();

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
	// End of UObject interface

	// Field Interface
	virtual void PostDuplicate(const FField& InField) override;
	virtual FField* GetInnerFieldByName(const FName& InName) override;
	virtual void GetInnerFields(TArray<FField*>& OutFields) override;

	// UField interface
	virtual void AddCppProperty(FProperty* Property) override;
	// End of UField interface

	// FProperty interface
	virtual FString GetCPPMacroType(FString& ExtendedTypeText) const  override;
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical(const void* A, const void* B, uint32 PortFlags) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL) const override;
	virtual void ExportTextItem(FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const override;
	virtual const TCHAR* ImportText_Internal(const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText) const override;
	virtual void InitializeValueInternal(void* Dest) const override
	{
		if (EnumHasAnyFlags(MapFlags, EMapPropertyFlags::UsesMemoryImageAllocator))
		{
			checkf(false, TEXT("FFreezableScriptMap is not supported at the moment"));

			for (int32 i = 0; i < this->ArrayDim; ++i)
			{
				new ((uint8*)Dest + i * this->ElementSize) FFreezableScriptMap;
			}
		}
		else
		{
			for (int32 i = 0; i < this->ArrayDim; ++i)
			{
				new ((uint8*)Dest + i * this->ElementSize) FScriptMap;
			}
		}
	}
	virtual void CopyValuesInternal(void* Dest, void const* Src, int32 Count) const override;
	virtual void ClearValueInternal(void* Data) const override;
	virtual void DestroyValueInternal(void* Dest) const override;
	virtual bool PassCPPArgsByRef() const override;
	virtual void InstanceSubobjects(void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph) override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	// End of FProperty interface

	FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& KeyTypeText, const FString& InKeyExtendedTypeText, const FString& ValueTypeText, const FString& InValueExtendedTypeText) const;

	/*
	 * Helper function to get the number of key/value pairs inside of a map. 
	 * Used by the garbage collector where for performance reasons the provided map pointer is not guarded
	 */
	int32 GetNum(void* InMap) const
	{
		return WithScriptMap(InMap, [](auto* Map) { return Map->Num(); });
	}

	/*
	 * Helper function to get the sizeof of the map's key/value pair.
	 * Used by the garbage collector.
	 */
	int32 GetPairStride() const
	{
		return MapLayout.SetLayout.Size;
	}

	/*
	 * Helper function to check if the specified index of a key/value pair in the underlying set is valid.
	 * Used by the garbage collector where for performance reasons the provided map pointer is not guarded
	 */
	bool IsValidIndex(void* InMap, int32 Index) const
	{
		return WithScriptMap(InMap, [Index](auto* Map) { return Map->IsValidIndex(Index); });
	}

	/*
	 * Helper function to get the pointer to a key/value pair at the specified index.
	 * Used by the garbage collector where for performance reasons the provided map pointer is not guarded
	 */
	uint8* GetPairPtr(void* InMap, int32 Index) const
	{
		return WithScriptMap(InMap, [this, Index](auto* Map) { return (uint8*)Map->GetData(Index, MapLayout); });
	}

	const FProperty* GetKeyProperty() const
	{
		return KeyProp;
	}

	const FProperty* GetValueProperty() const
	{
		return ValueProp;
	}
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
};

// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FScriptSet, FProperty> FSetProperty_Super;

class COREUOBJECT_API FSetProperty : public FSetProperty_Super
{
	DECLARE_FIELD(FSetProperty, FSetProperty_Super, CASTCLASS_FSetProperty)

	// Properties representing the key type and value type of the contained pairs
	FProperty*       ElementProp;
	FScriptSetLayout SetLayout;

public:
	typedef FSetProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FSetProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);
	FSetProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags);

#if WITH_EDITORONLY_DATA
	explicit FSetProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	virtual ~FSetProperty();

	// UObject interface
	virtual void Serialize(FArchive& Ar) override;
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
	virtual FString GetCPPMacroType(FString& ExtendedTypeText) const  override;
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical(const void* A, const void* B, uint32 PortFlags) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL) const override;
	virtual void ExportTextItem(FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const override;
	virtual const TCHAR* ImportText_Internal(const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText) const override;
	virtual void CopyValuesInternal(void* Dest, void const* Src, int32 Count) const override;
	virtual void ClearValueInternal(void* Data) const override;
	virtual void DestroyValueInternal(void* Dest) const override;
	virtual bool PassCPPArgsByRef() const override;
	virtual void InstanceSubobjects(void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph) override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	// End of FProperty interface

	FString GetCPPTypeCustom(FString* ExtendedTypeText, uint32 CPPExportFlags, const FString& ElementTypeText, const FString& InElementExtendedTypeText) const;

	/*
	 * Helper function to get the number of elements inside of a set.
	 * Used by the garbage collector where for performance reasons the provided set pointer is not guarded
	 */
	int32 GetNum(void* InSet) const
	{
		FScriptSet* Set = (FScriptSet*)InSet;
		return Set->Num();
	}

	/*
	 * Helper function to get the size of the set element.
	 * Used by the garbage collector.
	 */
	int32 GetStride() const
	{
		return SetLayout.Size;
	}

	/*
	 * Helper function to check if the specified index of an element is valid.
	 * Used by the garbage collector where for performance reasons the provided set pointer is not guarded
	 */
	bool IsValidIndex(void* InSet, int32 Index) const
	{
		FScriptSet* Set = (FScriptSet*)InSet;
		return Set->IsValidIndex(Index);
	}

	/*
	 * Helper function to get the pointer to an element at the specified index.
	 * Used by the garbage collector where for performance reasons the provided set pointer is not guarded
	 */
	uint8* GetElementPtr(void* InSet, int32 Index) const
	{
		FScriptSet* Set = (FScriptSet*)InSet;
		return (uint8*)Set->GetData(Index, SetLayout);
	}
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
};

/**
 * FScriptArrayHelper: Pseudo dynamic array. Used to work with array properties in a sensible way.
 **/
class FScriptArrayHelper
{
	enum EInternal { Internal };

	template <typename CallableType>
	auto WithScriptArray(CallableType&& Callable) const
	{
		if (!!(ArrayFlags & EArrayPropertyFlags::UsesMemoryImageAllocator))
		{
			return Callable(FreezableArray);
		}
		else
		{
			return Callable(HeapArray);
		}
	}

public:
	/**
	 *	Constructor, brings together a property and an instance of the property located in memory
	 *	@param	InProperty: the property associated with this memory
	 *	@param	InArray: pointer to raw memory that corresponds to this array. This can be NULL, and sometimes is, but in that case almost all operations will crash.
	**/
	FORCEINLINE FScriptArrayHelper(const FArrayProperty* InProperty, const void* InArray)
		: FScriptArrayHelper(Internal, InProperty->Inner, InArray, InProperty->Inner->ElementSize, InProperty->Inner->GetMinAlignment(), InProperty->ArrayFlags)
	{
	}

	/**
	 *	Index range check
	 *	@param	Index: Index to check
	 *	@return true if accessing this element is legal.
	**/
	FORCEINLINE bool IsValidIndex( int32 Index ) const
	{
		return Index >= 0 && Index < Num();
	}
	/**
	 *	Return the number of elements in the array.
	 *	@return	The number of elements in the array.
	**/
	FORCEINLINE int32 Num() const
	{
		int32 Result = WithScriptArray([](auto* Array) { return Array->Num(); });
		checkSlow(Result >= 0);
		return Result;
	}
	/**
	 *	Static version of Num() used when you don't need to bother to construct a FScriptArrayHelper. Returns the number of elements in the array.
	 *	@param	Target: pointer to the raw memory associated with a FScriptArray
	 *	@return The number of elements in the array.
	**/
	UE_DEPRECATED(4.25, "This shortcut is no longer valid - the Num() should be read from a proper array helper")
	static FORCEINLINE int32 Num(const void *Target)
	{
		checkSlow(((const FScriptArray*)Target)->Num() >= 0); 
		return ((const FScriptArray*)Target)->Num();
	}
	/**
	 *	Returns a uint8 pointer to an element in the array
	 *	@param	Index: index of the item to return a pointer to.
	 *	@return	Pointer to this element, or NULL if the array is empty
	**/
	FORCEINLINE uint8* GetRawPtr(int32 Index = 0)
	{
		if (!Num())
		{
			checkSlow(!Index);
			return NULL;
		}
		checkSlow(IsValidIndex(Index)); 
		return (uint8*)WithScriptArray([](auto* Array) { return Array->GetData(); }) + Index * ElementSize;
	}
	/**
	*	Empty the array, then add blank, constructed values to a given size.
	*	@param	Count: the number of items the array will have on completion.
	**/
	void EmptyAndAddValues(int32 Count)
	{ 
		check(Count>=0);
		checkSlow(Num() >= 0); 
		EmptyValues(Count);
		AddValues(Count);
	}
	/**
	*	Empty the array, then add uninitialized values to a given size.
	*	@param	Count: the number of items the array will have on completion.
	**/
	void EmptyAndAddUninitializedValues(int32 Count)
	{ 
		check(Count>=0);
		checkSlow(Num() >= 0); 
		EmptyValues(Count);
		AddUninitializedValues(Count);
	}
	/**
	*	Expand the array, if needed, so that the given index is valid
	*	@param	Index: index for the item that we want to ensure is valid
	*	@return true if expansion was necessary
	*	NOTE: This is not a count, it is an INDEX, so the final count will be at least Index+1 this matches the usage.
	**/
	bool ExpandForIndex(int32 Index)
	{ 
		check(Index>=0);
		checkSlow(Num() >= 0); 
		if (Index >= Num())
		{
			AddValues(Index - Num() + 1);
			return true;
		}
		return false;
	}
	/**
	*	Add or remove elements to set the array to a given size.
	*	@param	Count: the number of items the array will have on completion.
	**/
	void Resize(int32 Count)
	{ 
		check(Count>=0);
		int32 OldNum = Num();
		if (Count > OldNum)
		{
			AddValues(Count - OldNum);
		}
		else if (Count < OldNum)
		{
			RemoveValues(Count, OldNum - Count);
		}
	}
	/**
	*	Add blank, constructed values to the end of the array.
	*	@param	Count: the number of items to insert.
	*	@return	the index of the first newly added item.
	**/
	int32 AddValues(int32 Count)
	{ 
		const int32 OldNum = AddUninitializedValues(Count);		
		ConstructItems(OldNum, Count);
		return OldNum;
	}
	/**
	*	Add a blank, constructed values to the end of the array.
	*	@return	the index of the newly added item.
	**/
	FORCEINLINE int32 AddValue()
	{ 
		return AddValues(1);
	}
	/**
	*	Add uninitialized values to the end of the array.
	*	@param	Count: the number of items to insert.
	*	@return	the index of the first newly added item.
	**/
	int32 AddUninitializedValues(int32 Count)
	{
		check(Count>=0);
		checkSlow(Num() >= 0);
		const int32 OldNum = WithScriptArray([this, Count](auto* Array) { return Array->Add(Count, ElementSize, ElementAlignment); });
		return OldNum;
	}
	/**
	*	Add an uninitialized value to the end of the array.
	*	@return	the index of the newly added item.
	**/
	FORCEINLINE int32 AddUninitializedValue()
	{
		return AddUninitializedValues(1);
	}
	/**
	 *	Insert blank, constructed values into the array.
	 *	@param	Index: index of the first inserted item after completion
	 *	@param	Count: the number of items to insert.
	**/
	void InsertValues( int32 Index, int32 Count = 1)
	{
		check(Count>=0);
		check(Index>=0 && Index <= Num());
		WithScriptArray([this, Index, Count](auto* Array) { Array->Insert(Index, Count, ElementSize, ElementAlignment); });
		ConstructItems(Index, Count);
	}
	/**
	 *	Remove all values from the array, calling destructors, etc as appropriate.
	 *	@param Slack: used to presize the array for a subsequent add, to avoid reallocation.
	**/
	void EmptyValues(int32 Slack = 0)
	{
		checkSlow(Slack>=0);
		const int32 OldNum = Num();
		if (OldNum)
		{
			DestructItems(0, OldNum);
		}
		if (OldNum || Slack)
		{
			WithScriptArray([this, Slack](auto* Array) { Array->Empty(Slack, ElementSize, ElementAlignment); });
		}
	}
	/**
	 *	Remove values from the array, calling destructors, etc as appropriate.
	 *	@param Index: first item to remove.
	 *	@param Count: number of items to remove.
	**/
	void RemoveValues(int32 Index, int32 Count = 1)
	{
		check(Count>=0);
		check(Index>=0 && Index + Count <= Num());
		DestructItems(Index, Count);
		WithScriptArray([this, Index, Count](auto* Array) { Array->Remove(Index, Count, ElementSize, ElementAlignment); });
	}

	/**
	*	Clear values in the array. The meaning of clear is defined by the property system.
	*	@param Index: first item to clear.
	*	@param Count: number of items to clear.
	**/
	void ClearValues(int32 Index, int32 Count = 1)
	{
		check(Count>=0);
		check(Index>=0);
		ClearItems(Index, Count);
	}

	/**
	 *	Swap two elements in the array, does not call constructors and destructors
	 *	@param A index of one item to swap.
	 *	@param B index of the other item to swap.
	**/
	void SwapValues(int32 A, int32 B)
	{
		WithScriptArray([this, A, B](auto* Array) { Array->SwapMemory(A, B, ElementSize); });
	}

	/**
	 *	Move the allocation from another array and make it our own.
	 *	@note The arrays MUST be of the same type, and this function will NOT validate that!
	 *	@param InOtherArray The array to move the allocation from.
	**/
	void MoveAssign(void* InOtherArray)
	{
		checkSlow(InOtherArray);
		WithScriptArray([this, InOtherArray](auto* Array) { Array->MoveAssign(*static_cast<decltype(Array)>(InOtherArray), ElementSize, ElementAlignment); });
	}

	/**
	 *	Used by memory counting archives to accumulate the size of this array.
	 *	@param Ar archive to accumulate sizes
	**/
	void CountBytes( FArchive& Ar  ) const
	{
		WithScriptArray([this, &Ar](auto* Array) { Array->CountBytes(Ar, ElementSize); });
	}

	/**
	 * Destroys the container object - THERE SHOULD BE NO MORE USE OF THIS HELPER AFTER THIS FUNCTION IS CALLED!
	 */
	void DestroyContainer_Unsafe()
	{
		WithScriptArray([](auto* Array) { DestructItem(Array); });
	}

	static FScriptArrayHelper CreateHelperFormInnerProperty(const FProperty* InInnerProperty, const void *InArray, EArrayPropertyFlags InArrayFlags = EArrayPropertyFlags::None)
	{
		return FScriptArrayHelper(Internal, InInnerProperty, InArray, InInnerProperty->ElementSize, InInnerProperty->GetMinAlignment(), InArrayFlags);
	}

private:
	FScriptArrayHelper(EInternal, const FProperty* InInnerProperty, const void* InArray, int32 InElementSize, uint32 InElementAlignment, EArrayPropertyFlags InArrayFlags)
		: InnerProperty(InInnerProperty)
		, ElementSize(InElementSize)
		, ElementAlignment(InElementAlignment)
		, ArrayFlags(InArrayFlags)
	{
		//@todo, we are casting away the const here
		if (!!(InArrayFlags & EArrayPropertyFlags::UsesMemoryImageAllocator))
		{
			FreezableArray = (FFreezableScriptArray*)InArray;
		}
		else
		{
			HeapArray = (FScriptArray*)InArray;
		}

		check(ElementSize > 0);
		check(InnerProperty);
	}

	/**
	 *	Internal function to call into the property system to construct / initialize elements.
	 *	@param Index: first item to .
	 *	@param Count: number of items to .
	**/
	void ConstructItems(int32 Index, int32 Count)
	{
		checkSlow(Count >= 0);
		checkSlow(Index >= 0); 
		checkSlow(Index <= Num());
		checkSlow(Index + Count <= Num());
		if (Count > 0)
		{
			uint8* Dest = GetRawPtr(Index);
			if (InnerProperty->PropertyFlags & CPF_ZeroConstructor)
			{
				FMemory::Memzero(Dest, Count * ElementSize);
			}
			else
			{
				for (int32 LoopIndex = 0; LoopIndex < Count; LoopIndex++, Dest += ElementSize)
				{
					InnerProperty->InitializeValue(Dest);
				}
			}
		}
	}
	/**
	 *	Internal function to call into the property system to destruct elements.
	 *	@param Index: first item to .
	 *	@param Count: number of items to .
	**/
	void DestructItems(int32 Index, int32 Count)
	{
		if (!(InnerProperty->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor)))
		{
			checkSlow(Count >= 0);
			checkSlow(Index >= 0); 
			checkSlow(Index < Num());
			checkSlow(Index + Count <= Num());
			if (Count > 0)
			{
				uint8* Dest = GetRawPtr(Index);
				for (int32 LoopIndex = 0; LoopIndex < Count; LoopIndex++, Dest += ElementSize)
				{
					InnerProperty->DestroyValue(Dest);
				}
			}
		}
	}
	/**
	 *	Internal function to call into the property system to clear elements.
	 *	@param Index: first item to .
	 *	@param Count: number of items to .
	**/
	void ClearItems(int32 Index, int32 Count)
	{
		checkSlow(Count >= 0);
		checkSlow(Index >= 0); 
		checkSlow(Index < Num());
		checkSlow(Index + Count <= Num());
		if (Count > 0)
		{
			uint8* Dest = GetRawPtr(Index);
			if ((InnerProperty->PropertyFlags & (CPF_ZeroConstructor | CPF_NoDestructor)) == (CPF_ZeroConstructor | CPF_NoDestructor))
			{
				FMemory::Memzero(Dest, Count * ElementSize);
			}
			else
			{
				for (int32 LoopIndex = 0; LoopIndex < Count; LoopIndex++, Dest += ElementSize)
				{
					InnerProperty->ClearValue(Dest);
				}
			}
		}
	}

	const FProperty* InnerProperty;
	union
	{
		FScriptArray* HeapArray;
		FFreezableScriptArray* FreezableArray;
	};
	int32 ElementSize;
	uint32 ElementAlignment;
	EArrayPropertyFlags ArrayFlags;
};

class FScriptArrayHelper_InContainer : public FScriptArrayHelper
{
public:
	FORCEINLINE FScriptArrayHelper_InContainer(const FArrayProperty* InProperty, const void* InContainer, int32 FixedArrayIndex=0)
		:FScriptArrayHelper(InProperty, InProperty->ContainerPtrToValuePtr<void>(InContainer, FixedArrayIndex))
	{
	}

	FORCEINLINE FScriptArrayHelper_InContainer(const FArrayProperty* InProperty, const UObject* InContainer, int32 FixedArrayIndex=0)
		:FScriptArrayHelper(InProperty, InProperty->ContainerPtrToValuePtr<void>(InContainer, FixedArrayIndex))
	{
	}
};


/**
 * FScriptMapHelper: Pseudo dynamic map. Used to work with map properties in a sensible way.
 */
class FScriptMapHelper
{
	enum EInternal { Internal };

	friend class FMapProperty;

	template <typename CallableType>
	auto WithScriptMap(CallableType&& Callable) const
	{
		if (!!(MapFlags & EMapPropertyFlags::UsesMemoryImageAllocator))
		{
			return Callable(FreezableMap);
		}
		else
		{
			return Callable(HeapMap);
		}
	}

public:
	/**
	 * Constructor, brings together a property and an instance of the property located in memory
	 *
	 * @param  InProperty  The property associated with this memory
	 * @param  InMap       Pointer to raw memory that corresponds to this map. This can be NULL, and sometimes is, but in that case almost all operations will crash.
	 */
	FORCEINLINE FScriptMapHelper(const FMapProperty* InProperty, const void* InMap)
		: FScriptMapHelper(Internal, InProperty->KeyProp, InProperty->ValueProp, InMap, InProperty->MapLayout, InProperty->MapFlags)
	{
	}

	/**
	 * Index range check
	 *
	 * @param  Index  Index to check
	 *
	 * @return true if accessing this element is legal.
	 */
	FORCEINLINE bool IsValidIndex(int32 Index) const
	{
		return WithScriptMap([Index](auto* Map) { return Map->IsValidIndex(Index); });
	}

	/**
	 * Returns the number of elements in the map.
	 *
	 * @return The number of elements in the map.
	 */
	FORCEINLINE int32 Num() const
	{
		int32 Result = WithScriptMap([](auto* Map) { return Map->Num(); });
		checkSlow(Result >= 0); 
		return Result;
	}

	/**
	 * Returns the (non-inclusive) maximum index of elements in the map.
	 *
	 * @return The (non-inclusive) maximum index of elements in the map.
	 */
	FORCEINLINE int32 GetMaxIndex() const
	{
		return WithScriptMap([](auto* Map)
		{
			int32 Result = Map->GetMaxIndex();
			checkSlow(Result >= Map->Num());
			return Result;
		});
	}

	/**
	 * Static version of Num() used when you don't need to bother to construct a FScriptMapHelper. Returns the number of elements in the map.
	 *
	 * @param  Target  Pointer to the raw memory associated with a FScriptMap
	 *
	 * @return The number of elements in the map.
	 */
	UE_DEPRECATED(4.25, "This shortcut is no longer valid - the Num() should be read from a proper map helper")
	static FORCEINLINE int32 Num(const void* Target)
	{
		int32 Result = ((const FScriptMap*)Target)->Num();
		checkSlow(Result >= 0); 
		return Result;
	}

	/**
	 * Returns a uint8 pointer to the pair in the map
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the pair, or nullptr if the map is empty.
	 */
	FORCEINLINE uint8* GetPairPtr(int32 Index)
	{
		return WithScriptMap([this, Index](auto* Map) -> uint8*
		{
			if (Map->Num() == 0)
			{
				checkSlow(!Index);
				return nullptr;
			}

			checkSlow(Map->IsValidIndex(Index));
			return (uint8*)Map->GetData(Index, MapLayout);
		});
	}

	/**
	 * Returns a uint8 pointer to the Key (first element) in the map. Currently 
	 * identical to GetPairPtr, but provides clarity of purpose and avoids exposing
	 * implementation details of TMap.
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the key, or nullptr if the map is empty.
	 */
	FORCEINLINE uint8* GetKeyPtr(int32 Index)
	{
		return WithScriptMap([this, Index](auto* Map) -> uint8*
		{
			if (Map->Num() == 0)
			{
				checkSlow(!Index);
				return nullptr;
			}
		
			checkSlow(Map->IsValidIndex(Index));
			return (uint8*)Map->GetData(Index, MapLayout);
		});
	}

	/**
	 * Returns a uint8 pointer to the Value (second element) in the map.
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the value, or nullptr if the map is empty.
	 */
	FORCEINLINE uint8* GetValuePtr(int32 Index)
	{
		return WithScriptMap([this, Index](auto* Map) -> uint8*
		{
			if (Map->Num() == 0)
			{
				checkSlow(!Index);
				return nullptr;
			}
		
			checkSlow(Map->IsValidIndex(Index));
			return (uint8*)Map->GetData(Index, MapLayout) + MapLayout.ValueOffset;
		});
	}

	/**
	 * Returns a uint8 pointer to the pair in the map.
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the pair, or nullptr if the map is empty.
	 */
	FORCEINLINE const uint8* GetPairPtr(int32 Index) const
	{
		return const_cast<FScriptMapHelper*>(this)->GetPairPtr(Index);
	}

	/**
	 * Move the allocation from another map and make it our own.
	 * @note The maps MUST be of the same type, and this function will NOT validate that!
	 *
	 * @param InOtherMap The map to move the allocation from.
	 */
	void MoveAssign(void* InOtherMap)
	{
		checkSlow(InOtherMap);
		return WithScriptMap([this, InOtherMap](auto* Map)
		{
			Map->MoveAssign(*(decltype(Map))InOtherMap, MapLayout);
		});
	}

	/**
	 * Add an uninitialized value to the end of the map.
	 *
	 * @return  The index of the added element.
	 */
	FORCEINLINE int32 AddUninitializedValue()
	{
		return WithScriptMap([this](auto* Map)
		{
			checkSlow(Map->Num() >= 0);

			return Map->AddUninitialized(MapLayout);
		});
	}

	/**
	 *	Remove all values from the map, calling destructors, etc as appropriate.
	 *	@param Slack: used to presize the set for a subsequent add, to avoid reallocation.
	**/
	void EmptyValues(int32 Slack = 0)
	{
		checkSlow(Slack >= 0);

		int32 OldNum = Num();
		if (OldNum)
		{
			DestructItems(0, OldNum);
		}
		if (OldNum || Slack)
		{
			return WithScriptMap([this, Slack](auto* Map)
			{
				Map->Empty(Slack, MapLayout);
			});
		}
	}

	/**
	 * Adds a blank, constructed value to a given size.
	 * Note that this will create an invalid map because all the keys will be default constructed, and the map needs rehashing.
	 *
	 * @return  The index of the first element added.
	 **/
	int32 AddDefaultValue_Invalid_NeedsRehash()
	{
		return WithScriptMap([this](auto* Map)
		{
			checkSlow(Map->Num() >= 0);

			int32 Result = Map->AddUninitialized(MapLayout);
			ConstructItem(Result);

			return Result;
		});
	}

	/**
	 * Returns the property representing the key of the map pair.
	 *
	 * @return The property representing the key of the map pair.
	 */
	FProperty* GetKeyProperty() const
	{
		return KeyProp;
	}

	/**
	 * Returns the property representing the value of the map pair.
	 *
	 * @return The property representing the value of the map pair.
	 */
	FProperty* GetValueProperty() const
	{
		return ValueProp;
	}

	/**
	 * Removes an element at the specified index, destroying it.
	 *
	 * @param  Index  The index of the element to remove.
	 */
	void RemoveAt(int32 Index, int32 Count = 1)
	{
		return WithScriptMap([this, Index, Count](auto* Map)
		{
			check(Map->IsValidIndex(Index));

			DestructItems(Index, Count);
			for (int32 LocalCount = Count, LocalIndex = Index; LocalCount; ++LocalIndex)
			{
				if (Map->IsValidIndex(LocalIndex))
				{
					Map->RemoveAt(LocalIndex, MapLayout);
					--LocalCount;
				}
			}
		});
	}

	/**
	 * Rehashes the keys in the map.
	 * This function must be called to create a valid map.
	 */
	COREUOBJECT_API void Rehash();

	/** 
	 * Maps have gaps in their indices, so this function translates a logical index (ie. Nth element) 
	 * to an internal index that can be used for the other functions in this class.
	 * NOTE: This is slow, do not use this for iteration!
	 */
	int32 FindInternalIndex(int32 LogicalIdx) const
	{
		return WithScriptMap([this, LogicalIdx](auto* Map) -> int32
		{
			int32 LocalLogicalIdx = LogicalIdx;
			if (LocalLogicalIdx < 0 && LocalLogicalIdx > Map->Num())
			{
				return INDEX_NONE;
			}

			int32 MaxIndex = Map->GetMaxIndex();
			for (int32 Actual = 0; Actual < MaxIndex; ++Actual)
			{
				if (Map->IsValidIndex(Actual))
				{
					if (LocalLogicalIdx == 0)
					{
						return Actual;
					}
					--LocalLogicalIdx;
				}
			}
			return INDEX_NONE;
		});
	}

	/**
	 * Finds the index of an element in a map which matches the key in another pair.
	 *
	 * @param  PairWithKeyToFind  The address of a map pair which contains the key to search for.
	 * @param  IndexHint          The index to start searching from.
	 *
	 * @return The index of an element found in MapHelper, or -1 if none was found.
	 */
	int32 FindMapIndexWithKey(const void* PairWithKeyToFind, int32 IndexHint = 0) const
	{
		return WithScriptMap([this, PairWithKeyToFind, &IndexHint](auto* Map) -> int32
		{
			int32 MapMax = Map->GetMaxIndex();
			if (MapMax == 0)
			{
				return INDEX_NONE;
			}

			if (IndexHint >= MapMax)
			{
				IndexHint = 0;
			}

			check(IndexHint >= 0);

			FProperty* LocalKeyProp = this->KeyProp; // prevent aliasing in loop below

			int32 Index = IndexHint;
			for (;;)
			{
				if (Map->IsValidIndex(Index))
				{
					const void* PairToSearch = Map->GetData(Index, MapLayout);
					if (LocalKeyProp->Identical(PairWithKeyToFind, PairToSearch))
					{
						return Index;
					}
				}

				++Index;
				if (Index == MapMax)
				{
					Index = 0;
				}

				if (Index == IndexHint)
				{
					return INDEX_NONE;
				}
			}
		});
	}

	/**
	 * Finds the pair in a map which matches the key in another pair.
	 *
	 * @param  PairWithKeyToFind  The address of a map pair which contains the key to search for.
	 * @param  IndexHint          The index to start searching from.
	 *
	 * @return A pointer to the found pair, or nullptr if none was found.
	 */
	FORCEINLINE uint8* FindMapPairPtrWithKey(const void* PairWithKeyToFind, int32 IndexHint = 0)
	{
		int32 Index = FindMapIndexWithKey(PairWithKeyToFind, IndexHint);
		uint8* Result = (Index >= 0) ? GetPairPtr(Index) : nullptr;
		return Result;
	}

	/** Finds the associated pair from hash, rather than linearly searching */
	uint8* FindMapPairPtrFromHash(const void* KeyPtr)
	{
		int32 Index = WithScriptMap([this, KeyPtr, LocalKeyPropForCapture = this->KeyProp](auto* Map)
		{
			return Map->FindPairIndex(
				KeyPtr,
				MapLayout,
				[LocalKeyPropForCapture](const void* ElementKey) { return LocalKeyPropForCapture->GetValueTypeHash(ElementKey); },
				[LocalKeyPropForCapture](const void* A, const void* B) { return LocalKeyPropForCapture->Identical(A, B); }
			);
		});
		uint8* Result = (Index >= 0) ? GetPairPtr(Index) : nullptr;
		return Result;
	}

	/** Finds the associated value from hash, rather than linearly searching */
	uint8* FindValueFromHash(const void* KeyPtr)
	{
		return WithScriptMap([this, KeyPtr, LocalKeyPropForCapture = this->KeyProp](auto* Map)
		{
			return Map->FindValue(
				KeyPtr,
				MapLayout,
				[LocalKeyPropForCapture](const void* ElementKey) { return LocalKeyPropForCapture->GetValueTypeHash(ElementKey); },
				[LocalKeyPropForCapture](const void* A, const void* B) { return LocalKeyPropForCapture->Identical(A, B); }
			);
		});
	}

	/** Adds the (key, value) pair to the map, returning true if the element was added, or false if the element was already present and has been overwritten */
	void AddPair(const void* KeyPtr, const void* ValuePtr)
	{
		return WithScriptMap([this, KeyPtr, ValuePtr, LocalKeyPropForCapture = this->KeyProp, LocalValuePropForCapture = this->ValueProp](auto* Map)
		{
			Map->Add(
				KeyPtr,
				ValuePtr,
				MapLayout,
				[LocalKeyPropForCapture](const void* ElementKey) { return LocalKeyPropForCapture->GetValueTypeHash(ElementKey); },
				[LocalKeyPropForCapture](const void* A, const void* B) { return LocalKeyPropForCapture->Identical(A, B); },
				[LocalKeyPropForCapture, KeyPtr](void* NewElementKey)
				{
					if (LocalKeyPropForCapture->PropertyFlags & CPF_ZeroConstructor)
					{
						FMemory::Memzero(NewElementKey, LocalKeyPropForCapture->GetSize());
					}
					else
					{
						LocalKeyPropForCapture->InitializeValue(NewElementKey);
					}

					LocalKeyPropForCapture->CopySingleValueToScriptVM(NewElementKey, KeyPtr);
				},
				[LocalValuePropForCapture, ValuePtr](void* NewElementValue)
				{
					if (LocalValuePropForCapture->PropertyFlags & CPF_ZeroConstructor)
					{
						FMemory::Memzero(NewElementValue, LocalValuePropForCapture->GetSize());
					}
					else
					{
						LocalValuePropForCapture->InitializeValue(NewElementValue);
					}

					LocalValuePropForCapture->CopySingleValueToScriptVM(NewElementValue, ValuePtr);
				},
				[LocalValuePropForCapture, ValuePtr](void* ExistingElementValue)
				{
					LocalValuePropForCapture->CopySingleValueToScriptVM(ExistingElementValue, ValuePtr);
				},
				[LocalKeyPropForCapture](void* ElementKey)
				{
					if (!(LocalKeyPropForCapture->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor)))
					{
						LocalKeyPropForCapture->DestroyValue(ElementKey);
					}
				},
				[LocalValuePropForCapture](void* ElementValue)
				{
					if (!(LocalValuePropForCapture->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor)))
					{
						LocalValuePropForCapture->DestroyValue(ElementValue);
					}
				}
			);
		});
	}


	/**
	 * Finds or adds a new default-constructed value
	 *
	 * No need to rehash after calling. The hash table must be properly hashed before calling.
	 *
	 * @return The address to the value, not the pair
	 **/
	void* FindOrAdd(const void* KeyPtr)
	{
		return WithScriptMap([this, KeyPtr, LocalKeyPropForCapture = this->KeyProp, LocalValuePropForCapture = this->ValueProp](auto* Map)
		{
			return Map->FindOrAdd(
				KeyPtr,
				MapLayout,
				[LocalKeyPropForCapture](const void* ElementKey) { return LocalKeyPropForCapture->GetValueTypeHash(ElementKey); },
				[LocalKeyPropForCapture](const void* A, const void* B) { return LocalKeyPropForCapture->Identical(A, B); },
				[LocalKeyPropForCapture, LocalValuePropForCapture, KeyPtr](void* NewElementKey, void* NewElementValue)
				{
					if (LocalKeyPropForCapture->PropertyFlags & CPF_ZeroConstructor)
					{
						FMemory::Memzero(NewElementKey, LocalKeyPropForCapture->GetSize());
					}
					else
					{
						LocalKeyPropForCapture->InitializeValue(NewElementKey);
					}

					LocalKeyPropForCapture->CopySingleValue(NewElementKey, KeyPtr);

					if (LocalValuePropForCapture->PropertyFlags & CPF_ZeroConstructor)
					{
						FMemory::Memzero(NewElementValue, LocalValuePropForCapture->GetSize());
					}
					else
					{
						LocalValuePropForCapture->InitializeValue(NewElementValue);
					}
				}
			);
		});
	}


	/** Removes the key and its associated value from the map */
	bool RemovePair(const void* KeyPtr)
	{
		return WithScriptMap([this, KeyPtr, LocalKeyPropForCapture = this->KeyProp](auto* Map)
		{
			if (uint8* Entry = Map->FindValue(
				KeyPtr,
				MapLayout,
				[LocalKeyPropForCapture](const void* ElementKey) { return LocalKeyPropForCapture->GetValueTypeHash(ElementKey); },
				[LocalKeyPropForCapture](const void* A, const void* B) { return LocalKeyPropForCapture->Identical(A, B); }
			))
			{
				int32 Idx = (int32)((Entry - (uint8*)Map->GetData(0, MapLayout)) / MapLayout.SetLayout.Size);
				RemoveAt(Idx);
				return true;
			}
			else
			{
				return false;
			}
		});
	}

	/**
	 * Checks if a key in the map matches the specified key
	 *
	 * @param	InBaseAddress	The base address of the map
	 * @param	InKeyValue		The key to find within the map
	 *
	 * @return	True if the key is found, false otherwise
	 */
	bool HasKey(const void* InBaseAddress, const FString& InKeyValue) const
	{
		for (int32 Index = 0, ItemsLeft = Num(); ItemsLeft > 0; ++Index)
		{
			if (IsValidIndex(Index))
			{
				--ItemsLeft;

				const uint8* PairPtr = GetPairPtr(Index);
				const uint8* KeyPtr = KeyProp->ContainerPtrToValuePtr<const uint8>(PairPtr);

				FString KeyValue;
				if (KeyPtr != InBaseAddress && KeyProp->ExportText_Direct(KeyValue, KeyPtr, KeyPtr, nullptr, 0))
				{
					if ((CastField<FObjectProperty>(KeyProp) != nullptr && KeyValue.Contains(InKeyValue)) || InKeyValue == KeyValue)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	static FScriptMapHelper CreateHelperFormInnerProperties(FProperty* InKeyProperty, FProperty* InValProperty, const void *InMap, EMapPropertyFlags InMapFlags = EMapPropertyFlags::None)
	{
		return FScriptMapHelper(
			Internal,
			InKeyProperty,
			InValProperty,
			InMap,
			FScriptMap::GetScriptLayout(InKeyProperty->GetSize(), InKeyProperty->GetMinAlignment(), InValProperty->GetSize(), InValProperty->GetMinAlignment()),
			InMapFlags
		);
	}

	class FIterator
	{
	public:
		explicit FIterator(const FScriptMapHelper& InMap) :
			Map(InMap),
			CurrentIndex(-1)
		{
			Advance();
		}

		FIterator& operator++() { Advance(); return *this; }
		FIterator& operator++(int) { Advance(); return *this; }
		explicit operator bool() const { return Map.IsValidIndex(CurrentIndex); }
		int32 operator*() const { return CurrentIndex; }

	private:
		const FScriptMapHelper& Map;
		int32 CurrentIndex;

		void Advance()
		{
			++CurrentIndex;
			while (CurrentIndex < Map.GetMaxIndex() && !Map.IsValidIndex(CurrentIndex))
			{
				++CurrentIndex;
			}
		}
	};

	FScriptMapHelper::FIterator CreateIterator() const
	{
		return FIterator(*this);
	}

private:
	FORCEINLINE FScriptMapHelper(EInternal, FProperty* InKeyProp, FProperty* InValueProp, const void* InMap, const FScriptMapLayout& InMapLayout, EMapPropertyFlags InMapFlags)
		: KeyProp  (InKeyProp)
		, ValueProp(InValueProp)
		, MapLayout(InMapLayout)
		, MapFlags (InMapFlags)
	{
		check(InKeyProp && InValueProp);

		//@todo, we are casting away the const here
		if (!!(InMapFlags & EMapPropertyFlags::UsesMemoryImageAllocator))
		{
			FreezableMap = (FFreezableScriptMap*)InMap;
		}
		else
		{
			HeapMap = (FScriptMap*)InMap;
		}

		check(KeyProp && ValueProp);
	}

	/**
	 * Internal function to call into the property system to construct / initialize elements.
	 *
	 * @param  Index  First item to construct.
	 * @param  Count  Number of items to construct.
	 */
	void ConstructItem(int32 Index)
	{
		check(IsValidIndex(Index));

		bool bZeroKey   = !!(KeyProp  ->PropertyFlags & CPF_ZeroConstructor);
		bool bZeroValue = !!(ValueProp->PropertyFlags & CPF_ZeroConstructor);

		void* Dest = WithScriptMap([this, Index](auto* Map) { return Map->GetData(Index, MapLayout); });

		if (bZeroKey || bZeroValue)
		{
			// If any nested property needs zeroing, just pre-zero the whole space
			FMemory::Memzero(Dest, MapLayout.SetLayout.Size);
		}

		if (!bZeroKey)
		{
			KeyProp->InitializeValue_InContainer(Dest);
		}

		if (!bZeroValue)
		{
			ValueProp->InitializeValue_InContainer(Dest);
		}
	}

	/**
	 * Internal function to call into the property system to destruct elements.
	 */
	void DestructItems(int32 Index, int32 Count)
	{
		check(Index >= 0);
		check(Count >= 0);

		if (Count == 0)
		{
			return;
		}

		bool bDestroyKeys   = !(KeyProp  ->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor));
		bool bDestroyValues = !(ValueProp->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor));

		if (bDestroyKeys || bDestroyValues)
		{
			uint32 Stride  = MapLayout.SetLayout.Size;
			uint8* PairPtr = WithScriptMap([this, Index](auto* Map) { return (uint8*)Map->GetData(Index, MapLayout); });
			if (bDestroyKeys)
			{
				if (bDestroyValues)
				{
					for (; Count; ++Index)
					{
						if (IsValidIndex(Index))
						{
							KeyProp  ->DestroyValue_InContainer(PairPtr);
							ValueProp->DestroyValue_InContainer(PairPtr);
							--Count;
						}
						PairPtr += Stride;
					}
				}
				else
				{
					for (; Count; ++Index)
					{
						if (IsValidIndex(Index))
						{
							KeyProp->DestroyValue_InContainer(PairPtr);
							--Count;
						}
						PairPtr += Stride;
					}
				}
			}
			else
			{
				for (; Count; ++Index)
				{
					if (IsValidIndex(Index))
					{
						ValueProp->DestroyValue_InContainer(PairPtr);
						--Count;
					}
					PairPtr += Stride;
				}
			}
		}
	}

	/**
	 * Returns a uint8 pointer to the pair in the array without checking the index.
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the pair, or nullptr if the map is empty.
	 */
	FORCEINLINE uint8* GetPairPtrWithoutCheck(int32 Index)
	{
		return WithScriptMap([this, Index](auto* Map) { return (uint8*)Map->GetData(Index, MapLayout); });
	}

	/**
	 * Returns a uint8 pointer to the pair in the array without checking the index.
	 *
	 * @param  Index  index of the item to return a pointer to.
	 *
	 * @return Pointer to the pair, or nullptr if the map is empty.
	 */
	FORCEINLINE const uint8* GetPairPtrWithoutCheck(int32 Index) const
	{
		return const_cast<FScriptMapHelper*>(this)->GetPairPtrWithoutCheck(Index);
	}

public:
	FProperty*        KeyProp;
	FProperty*        ValueProp;
	union
	{
		FScriptMap*          HeapMap;
		FFreezableScriptMap* FreezableMap;
	};
	FScriptMapLayout  MapLayout;
	EMapPropertyFlags MapFlags;
};

class FScriptMapHelper_InContainer : public FScriptMapHelper
{
public:
	FORCEINLINE FScriptMapHelper_InContainer(const FMapProperty* InProperty, const void* InArray, int32 FixedArrayIndex=0)
		:FScriptMapHelper(InProperty, InProperty->ContainerPtrToValuePtr<void>(InArray, FixedArrayIndex))
	{
	}
};

/**
* FScriptSetHelper: Pseudo dynamic Set. Used to work with Set properties in a sensible way.
*/
class FScriptSetHelper
{
	friend class FSetProperty;

public:
	/**
	* Constructor, brings together a property and an instance of the property located in memory
	*
	* @param  InProperty  The property associated with this memory
	* @param  InSet       Pointer to raw memory that corresponds to this Set. This can be NULL, and sometimes is, but in that case almost all operations will crash.
	*/
	FORCEINLINE FScriptSetHelper(const FSetProperty* InProperty, const void* InSet)
		: ElementProp(InProperty->ElementProp)
		, Set((FScriptSet*)InSet)  //@todo, we are casting away the const here
		, SetLayout(InProperty->SetLayout)
	{
		check(ElementProp);
	}

	/**
	* Index range check
	*
	* @param  Index  Index to check
	*
	* @return true if accessing this element is legal.
	*/
	FORCEINLINE bool IsValidIndex(int32 Index) const
	{
		return Set->IsValidIndex(Index);
	}

	/**
	* Returns the number of elements in the set.
	*
	* @return The number of elements in the set.
	*/
	FORCEINLINE int32 Num() const
	{
		const int32 Result = Set->Num();
		checkSlow(Result >= 0); 
		return Result;
	}

	/**
	* Returns the (non-inclusive) maximum index of elements in the set.
	*
	* @return The (non-inclusive) maximum index of elements in the set.
	*/
	FORCEINLINE int32 GetMaxIndex() const
	{
		const int32 Result = Set->GetMaxIndex();
		checkSlow(Result >= Num());
		return Result;
	}

	/**
	* Static version of Num() used when you don't need to bother to construct a FScriptSetHelper. Returns the number of elements in the set.
	*
	* @param  Target  Pointer to the raw memory associated with a FScriptSet
	*
	* @return The number of elements in the set.
	*/
	static FORCEINLINE int32 Num(const void* Target)
	{
		const int32 Result = ((const FScriptSet*)Target)->Num();
		checkSlow(Result >= 0); 
		return Result;
	}

	/**
	* Returns a uint8 pointer to the element in the set.
	*
	* @param  Index  index of the item to return a pointer to.
	*
	* @return Pointer to the element, or nullptr if the set is empty.
	*/
	FORCEINLINE uint8* GetElementPtr(int32 Index)
	{
		if (Num() == 0)
		{
			checkSlow(!Index);
			return nullptr;
		}

		checkSlow(IsValidIndex(Index));
		return (uint8*)Set->GetData(Index, SetLayout);
	}

	/**
	* Returns a uint8 pointer to the element in the set.
	*
	* @param  Index  index of the item to return a pointer to.
	*
	* @return Pointer to the element, or nullptr if the set is empty.
	*/
	FORCEINLINE const uint8* GetElementPtr(int32 Index) const
	{
		return const_cast<FScriptSetHelper*>(this)->GetElementPtr(Index);
	}

	/**
	* Move the allocation from another set and make it our own.
	* @note The sets MUST be of the same type, and this function will NOT validate that!
	*
	* @param InOtherSet The set to move the allocation from.
	*/
	void MoveAssign(void* InOtherSet)
	{
		FScriptSet* OtherSet = (FScriptSet*)InOtherSet;
		checkSlow(OtherSet);
		Set->MoveAssign(*OtherSet, SetLayout);
	}

	/**
	* Add an uninitialized value to the end of the set.
	*
	* @return  The index of the added element.
	*/
	FORCEINLINE int32 AddUninitializedValue()
	{
		checkSlow(Num() >= 0);

		return Set->AddUninitialized(SetLayout);
	}

	/**
	*	Remove all values from the set, calling destructors, etc as appropriate.
	*	@param Slack: used to presize the set for a subsequent add, to avoid reallocation.
	**/
	void EmptyElements(int32 Slack = 0)
	{
		checkSlow(Slack >= 0);

		int32 OldNum = Num();
		if (OldNum)
		{
			DestructItems(0, OldNum);
		}
		if (OldNum || Slack)
		{
			Set->Empty(Slack, SetLayout);
		}
	}

	/**
	* Adds a blank, constructed value to a given size.
	* Note that this will create an invalid Set because all the keys will be default constructed, and the set needs rehashing.
	*
	* @return  The index of the first element added.
	**/
	int32 AddDefaultValue_Invalid_NeedsRehash()
	{
		checkSlow(Num() >= 0);

		int32 Result = AddUninitializedValue();
		ConstructItem(Result);

		return Result;
	}

	/**
	* Returns the property representing the element of the set
	*/
	FProperty* GetElementProperty() const
	{
		return ElementProp;
	}

	/**
	* Removes an element at the specified index, destroying it.
	*
	* @param  Index  The index of the element to remove.
	*/
	void RemoveAt(int32 Index, int32 Count = 1)
	{
		check(IsValidIndex(Index));

		DestructItems(Index, Count);
		for (; Count; ++Index)
		{
			if (IsValidIndex(Index))
			{
				Set->RemoveAt(Index, SetLayout);
				--Count;
			}
		}
	}

	/**
	* Rehashes the keys in the set.
	* This function must be called to create a valid set.
	*/
	COREUOBJECT_API void Rehash();

	/**
	 * Maps have gaps in their indices, so this function translates a logical index (ie. Nth element)
	 * to an internal index that can be used for the other functions in this class.
	 * NOTE: This is slow, do not use this for iteration!
	 */
	int32 FindInternalIndex(int32 LogicalIdx) const
	{
		if (LogicalIdx < 0 && LogicalIdx > Num())
		{
			return INDEX_NONE;
		}

		int32 MaxIndex = GetMaxIndex();
		for (int32 Actual = 0; Actual < MaxIndex; ++Actual)
		{
			if (IsValidIndex(Actual))
			{
				if (LogicalIdx == 0)
				{
					return Actual;
				}
				--LogicalIdx;
			}
		}
		return INDEX_NONE;
	}
	
	/**
	* Finds the index of an element in a set
	*
	* @param  ElementToFind		The address of an element to search for.
	* @param  IndexHint         The index to start searching from.
	*
	* @return The index of an element found in SetHelper, or -1 if none was found.
	*/
	int32 FindElementIndex(const void* ElementToFind, int32 IndexHint = 0) const
	{
		const int32 SetMax = GetMaxIndex();
		if (SetMax == 0)
		{
			return INDEX_NONE;
		}

		if (IndexHint >= SetMax)
		{
			IndexHint = 0;
		}

		check(IndexHint >= 0);

		FProperty* LocalKeyProp = this->ElementProp; // prevent aliasing in loop below

		int32 Index = IndexHint;
		for (;;)
		{
			if (IsValidIndex(Index))
			{
				const void* ElementToCheck = GetElementPtrWithoutCheck(Index);
				if (LocalKeyProp->Identical(ElementToFind, ElementToCheck))
				{
					return Index;
				}
			}

			++Index;
			if (Index == SetMax)
			{
				Index = 0;
			}

			if (Index == IndexHint)
			{
				return INDEX_NONE;
			}
		}
	}

	/**
	* Finds the pair in a map which matches the key in another pair.
	*
	* @param  PairWithKeyToFind  The address of a map pair which contains the key to search for.
	* @param  IndexHint          The index to start searching from.
	*
	* @return A pointer to the found pair, or nullptr if none was found.
	*/
	FORCEINLINE uint8* FindElementPtr(const void* ElementToFind, int32 IndexHint = 0)
	{
		const int32 Index = FindElementIndex(ElementToFind, IndexHint);
		uint8* Result = (Index >= 0 ? GetElementPtr(Index) : nullptr);
		return Result;
	}

	/** Finds element index from hash, rather than linearly searching */
	FORCEINLINE int32 FindElementIndexFromHash(const void* ElementToFind) const
	{
		FProperty* LocalElementPropForCapture = ElementProp;
		return Set->FindIndex(
			ElementToFind,
			SetLayout,
			[LocalElementPropForCapture](const void* Element) { return LocalElementPropForCapture->GetValueTypeHash(Element); },
			[LocalElementPropForCapture](const void* A, const void* B) { return LocalElementPropForCapture->Identical(A, B); }
		);
	}

	/** Finds element pointer from hash, rather than linearly searching */
	FORCEINLINE uint8* FindElementPtrFromHash(const void* ElementToFind)
	{
		const int32 Index = FindElementIndexFromHash(ElementToFind);
		uint8* Result = (Index >= 0 ? GetElementPtr(Index) : nullptr);
		return Result;
	}

	/** Adds the element to the set, returning true if the element was added, or false if the element was already present */
	void AddElement(const void* ElementToAdd)
	{
		FProperty* LocalElementPropForCapture = ElementProp;
		FScriptSetLayout& LocalSetLayoutForCapture = SetLayout;
		Set->Add(
			ElementToAdd,
			SetLayout,
			[LocalElementPropForCapture](const void* Element) { return LocalElementPropForCapture->GetValueTypeHash(Element); },
			[LocalElementPropForCapture](const void* A, const void* B) { return LocalElementPropForCapture->Identical(A, B); },
			[LocalElementPropForCapture, ElementToAdd, LocalSetLayoutForCapture](void* NewElement)
			{
				if (LocalElementPropForCapture->PropertyFlags & CPF_ZeroConstructor)
				{
					FMemory::Memzero(NewElement, LocalElementPropForCapture->GetSize());
				}
				else
				{
					LocalElementPropForCapture->InitializeValue(NewElement);
				}

				LocalElementPropForCapture->CopySingleValueToScriptVM(NewElement, ElementToAdd);
			},
			[LocalElementPropForCapture](void* Element)
			{
				if (!(LocalElementPropForCapture->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor)))
				{
					LocalElementPropForCapture->DestroyValue(Element);
				}
			}
		);
	}

	/** Removes the element from the set */
	bool RemoveElement(const void* ElementToRemove)
	{
		FProperty* LocalElementPropForCapture = ElementProp;
		int32 FoundIndex = Set->FindIndex(
			ElementToRemove,
			SetLayout,
			[LocalElementPropForCapture](const void* Element) { return LocalElementPropForCapture->GetValueTypeHash(Element); },
			[LocalElementPropForCapture](const void* A, const void* B) { return LocalElementPropForCapture->Identical(A, B); }
		);
		if (FoundIndex != INDEX_NONE)
		{
			RemoveAt(FoundIndex);
			return true;
		}
		else
		{
			return false;
		}
	}

	/**
	 * Checks if an element has already been added to the set
	 *
	 * @param	InBaseAddress	The base address of the set
	 * @param	InElementValue	The element value to check for
	 *
	 * @return	True if the element is found in the set, false otherwise
	 */
	bool HasElement(void* InBaseAddress, const FString& InElementValue) const
	{
		for (int32 Index = 0, ItemsLeft = Num(); ItemsLeft > 0; ++Index)
		{
			if (IsValidIndex(Index))
			{
				--ItemsLeft;

				const uint8* Element = GetElementPtr(Index);

				FString ElementValue;
				if (Element != InBaseAddress && ElementProp->ExportText_Direct(ElementValue, Element, Element, nullptr, 0))
				{
					if ((CastField<FObjectProperty>(ElementProp) != nullptr && ElementValue.Contains(InElementValue)) || ElementValue == InElementValue)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	static FScriptSetHelper CreateHelperFormElementProperty(FProperty* InElementProperty, const void *InSet)
	{
		check(InElementProperty);

		FScriptSetHelper ScriptSetHelper;
		ScriptSetHelper.ElementProp = InElementProperty;
		ScriptSetHelper.Set = (FScriptSet*)InSet;

		const int32 ElementPropSize = InElementProperty->GetSize();
		const int32 ElementPropAlignment = InElementProperty->GetMinAlignment();
		ScriptSetHelper.SetLayout = FScriptSet::GetScriptLayout(ElementPropSize, ElementPropAlignment);

		return ScriptSetHelper;
	}

	class FIterator
	{
	public:
		explicit FIterator(const FScriptSetHelper& InSet) :
			Set(InSet),
			CurrentIndex(-1)
		{
			Advance();
		}

		FIterator& operator++() { Advance(); return *this; }
		FIterator& operator++(int) { Advance(); return *this; }
		explicit operator bool() const { return Set.IsValidIndex(CurrentIndex); }
		int32 operator*() const { return CurrentIndex; }

	private:
		const FScriptSetHelper& Set;
		int32 CurrentIndex;

		void Advance()
		{
			++CurrentIndex;
			while (CurrentIndex < Set.GetMaxIndex() && !Set.IsValidIndex(CurrentIndex))
			{
				++CurrentIndex;
			}
		}
	};

	FScriptSetHelper::FIterator CreateIterator() const
	{
		return FIterator(*this);
	}

private: 
	FScriptSetHelper()
		: ElementProp(nullptr)
		, Set(nullptr)
		, SetLayout(FScriptSet::GetScriptLayout(0, 1))
	{}

	/**
	* Internal function to call into the property system to construct / initialize elements.
	*
	* @param  Index  First item to construct.
	* @param  Count  Number of items to construct.
	*/
	void ConstructItem(int32 Index)
	{
		check(IsValidIndex(Index));

		bool bZeroElement = !!(ElementProp->PropertyFlags & CPF_ZeroConstructor);
		uint8* Dest = GetElementPtrWithoutCheck(Index);

		if (bZeroElement)
		{
			// If any nested property needs zeroing, just pre-zero the whole space
			FMemory::Memzero(Dest, SetLayout.Size);
		}

		if (!bZeroElement)
		{
			ElementProp->InitializeValue_InContainer(Dest);
		}
	}

	/**
	* Internal function to call into the property system to destruct elements.
	*/
	void DestructItems(int32 Index, int32 Count)
	{
		check(Index >= 0);
		check(Count >= 0);

		if (Count == 0)
		{
			return;
		}

		bool bDestroyElements = !(ElementProp->PropertyFlags & (CPF_IsPlainOldData | CPF_NoDestructor));

		if (bDestroyElements)
		{
			uint32 Stride = SetLayout.Size;
			uint8* ElementPtr = GetElementPtrWithoutCheck(Index);

			for (; Count; ++Index)
			{
				if (IsValidIndex(Index))
				{
					ElementProp->DestroyValue_InContainer(ElementPtr);
					--Count;
				}
				ElementPtr += Stride;
			}
		}
	}

	/**
	* Returns a uint8 pointer to the element in the array without checking the index.
	*
	* @param  Index  index of the item to return a pointer to.
	*
	* @return Pointer to the element, or nullptr if the array is empty.
	*/
	FORCEINLINE uint8* GetElementPtrWithoutCheck(int32 Index)
	{
		return (uint8*)Set->GetData(Index, SetLayout);
	}

	/**
	* Returns a uint8 pointer to the element in the array without checking the index.
	*
	* @param  Index  index of the item to return a pointer to.
	*
	* @return Pointer to the pair, or nullptr if the array is empty.
	*/
	FORCEINLINE const uint8* GetElementPtrWithoutCheck(int32 Index) const
	{
		return const_cast<FScriptSetHelper*>(this)->GetElementPtrWithoutCheck(Index);
	}

public:
	FProperty*       ElementProp;
	FScriptSet*      Set;
	FScriptSetLayout SetLayout;
};

class FScriptSetHelper_InContainer : public FScriptSetHelper
{
public:
	FORCEINLINE FScriptSetHelper_InContainer(const FSetProperty* InProperty, const void* InArray, int32 FixedArrayIndex=0)
		:FScriptSetHelper(InProperty, InProperty->ContainerPtrToValuePtr<void>(InArray, FixedArrayIndex))
	{
	}
};

/*-----------------------------------------------------------------------------
	FStructProperty.
-----------------------------------------------------------------------------*/

//
// Describes a structure variable embedded in (as opposed to referenced by) 
// an object.
//
class COREUOBJECT_API FStructProperty : public FProperty
{
	DECLARE_FIELD(FStructProperty, FProperty, CASTCLASS_FStructProperty)

	// Variables.
	class UScriptStruct* Struct;
public:
	FStructProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);
	FStructProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UScriptStruct* InStruct);

#if WITH_EDITORONLY_DATA
	explicit FStructProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// FProperty interface
	virtual FString GetCPPMacroType( FString& ExtendedTypeText ) const  override;
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual void LinkInternal(FArchive& Ar) override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual bool SupportsNetSharedSerialization() const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual void CopyValuesInternal( void* Dest, void const* Src, int32 Count  ) const override;
	virtual void ClearValueInternal( void* Data ) const override;
	virtual void DestroyValueInternal( void* Dest ) const override;
	virtual void InitializeValueInternal( void* Dest ) const override;
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph ) override;
	virtual int32 GetMinAlignment() const override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface

	bool UseBinaryOrNativeSerialization(const FArchive& Ar) const;

private:
	virtual uint32 GetValueTypeHashInternal(const void* Src) const;

public:

#if HACK_HEADER_GENERATOR
	/**
	 * Some native structs, like FIntPoint, FIntRect, FVector2D, FVector, FPlane, FRotator, FCylinder have a default constructor that does nothing and require EForceInit
	 * Since it is name-based, this is not a fast routine intended to be used for header generation only
	 * 
	 * @return	true if this struct requires the EForceInit constructor to initialize
	 */
	bool HasNoOpConstructor() const;
#endif
};

/*-----------------------------------------------------------------------------
	FDelegateProperty.
-----------------------------------------------------------------------------*/

/**
 * Describes a pointer to a function bound to an Object.
 */
// need to break this out a different type so that the DECLARE_CASTED_CLASS_INTRINSIC macro can digest the comma
typedef TProperty<FScriptDelegate, FProperty> FDelegateProperty_Super;

class COREUOBJECT_API FDelegateProperty : public FDelegateProperty_Super
{
	DECLARE_FIELD(FDelegateProperty, FDelegateProperty_Super, CASTCLASS_FDelegateProperty)

	/** Points to the source delegate function (the function declared with the delegate keyword) used in the declaration of this delegate property. */
	UFunction* SignatureFunction;
public:

	typedef FDelegateProperty_Super::TTypeFundamentals TTypeFundamentals;
	typedef TTypeFundamentals::TCppType TCppType;

	FDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FDelegateProperty_Super(InOwner, InName, InObjectFlags)
		, SignatureFunction(nullptr)
	{
	}

	FDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = NULL)
		: FDelegateProperty_Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
		, SignatureFunction(InSignatureFunction)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FDelegateProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void BeginDestroy() override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// FProperty interface
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual const TCHAR* ImportText_Internal( const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText ) const override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph ) override;
	virtual bool SameType(const FProperty* Other) const override;
#if WITH_EDITORONLY_DATA
	virtual void AppendSchemaHash(FBlake3& Builder, bool bSkipEditorOnly) const override;
#endif
	// End of FProperty interface
};


/*-----------------------------------------------------------------------------
	FMulticastDelegateProperty.
-----------------------------------------------------------------------------*/

/**
 * Describes a list of functions bound to an Object.
 */
class COREUOBJECT_API FMulticastDelegateProperty : public FProperty
{
	DECLARE_FIELD(FMulticastDelegateProperty, FProperty, CASTCLASS_FMulticastDelegateProperty)

	/** Points to the source delegate function (the function declared with the delegate keyword) used in the declaration of this delegate property. */
	UFunction* SignatureFunction;

public:

	FMulticastDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: FProperty(InOwner, InName, InObjectFlags)
		, SignatureFunction(nullptr)
	{
	}

	FMulticastDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = NULL)
		: FProperty(InOwner, InName, InObjectFlags, InOffset, InFlags)
		, SignatureFunction(InSignatureFunction)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FMulticastDelegateProperty(UField* InField);
#endif // WITH_EDITORONLY_DATA

	// UObject interface
	virtual void Serialize( FArchive& Ar ) override;
	virtual void BeginDestroy() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of UObject interface

	// Field interface
	virtual void PostDuplicate(const FField& InField) override;

	// FProperty interface
	virtual FString GetCPPType( FString* ExtendedTypeText, uint32 CPPExportFlags ) const override;
	virtual FString GetCPPTypeForwardDeclaration() const override;
	virtual bool Identical( const void* A, const void* B, uint32 PortFlags ) const override;
	virtual bool NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data, TArray<uint8> * MetaData = NULL ) const override;
	virtual void ExportTextItem( FString& ValueStr, const void* PropertyValue, const void* DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope ) const override;
	virtual bool ContainsObjectReference(TArray<const FStructProperty*>& EncounteredStructProps, EPropertyObjectReferenceType InReferenceType = EPropertyObjectReferenceType::Strong) const override;
	virtual void EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps, FGCStackSizeHelper& StackSizeHelper) override;
	virtual void InstanceSubobjects( void* Data, void const* DefaultData, UObject* Owner, struct FObjectInstancingGraph* InstanceGraph ) override;
	virtual bool SameType(const FProperty* Other) const override;
	virtual EConvertFromTypeResult ConvertFromType(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct) override;
	// End of FProperty interface

	virtual const FMulticastScriptDelegate* GetMulticastDelegate(const void* PropertyValue) const PURE_VIRTUAL(FMulticastDelegateProperty::GetMulticastDelegate, return nullptr;);
	virtual void SetMulticastDelegate(void* PropertyValue, FMulticastScriptDelegate ScriptDelegate) const PURE_VIRTUAL(FMulticastDelegateProperty::SetMulticastDelegate, );

	virtual void AddDelegate(FScriptDelegate ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const PURE_VIRTUAL(FMulticastDelegateProperty::AddDelegate, );
	virtual void RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const PURE_VIRTUAL(FMulticastDelegateProperty::RemoveDelegate, );
	virtual void ClearDelegate(UObject* Parent = nullptr, void* PropertyValue = nullptr)  const PURE_VIRTUAL(FMulticastDelegateProperty::ClearDelegate, );

protected:
	friend class FProperty;

	static FMulticastScriptDelegate::FInvocationList EmptyList;
	virtual FMulticastScriptDelegate::FInvocationList& GetInvocationList(const void* PropertyValue) const PURE_VIRTUAL(FMulticastDelegateProperty::GetInvocationList, return EmptyList;);


	const TCHAR* ImportText_Add( const TCHAR* Buffer, void* PropertyValue, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const;
	const TCHAR* ImportText_Remove( const TCHAR* Buffer, void* PropertyValue, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText ) const;

	const TCHAR* ImportDelegateFromText(FMulticastScriptDelegate& MulticastDelegate, const TCHAR* Buffer, UObject* OwnerObject, FOutputDevice* ErrorText) const;
};

template<class InTCppType>
class COREUOBJECT_API TProperty_MulticastDelegate : public TProperty<InTCppType, FMulticastDelegateProperty>
{
public:
	typedef TProperty<InTCppType, FMulticastDelegateProperty> Super;
	typedef InTCppType TCppType;
	typedef typename Super::TTypeFundamentals TTypeFundamentals;
	TProperty_MulticastDelegate(FFieldVariant InOwner, const FName& InName, UFunction* InSignatureFunction = nullptr)
		: Super(InOwner, InName)
	{
		this->SignatureFunction = InSignatureFunction;
	}

	TProperty_MulticastDelegate(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: Super(InOwner, InName, InObjectFlags)
	{
		this->SignatureFunction = nullptr;
	}

	TProperty_MulticastDelegate(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
		: Super(InOwner, InName, InObjectFlags, InOffset, InFlags)
	{
		this->SignatureFunction = InSignatureFunction;
	}

	TProperty_MulticastDelegate(EInternal InInernal, FFieldClass* InClass)
		: Super(EC_InternalUseOnlyConstructor, InClass)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit TProperty_MulticastDelegate(UField* InField)
		: Super(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface.
	virtual FString GetCPPType(FString* ExtendedTypeText, uint32 CPPExportFlags) const override
	{
		return FMulticastDelegateProperty::GetCPPType(ExtendedTypeText, CPPExportFlags);
	}
	// End of FProperty interface
};

class COREUOBJECT_API FMulticastInlineDelegateProperty : public TProperty_MulticastDelegate<FMulticastScriptDelegate>
{
	DECLARE_FIELD(FMulticastInlineDelegateProperty, TProperty_MulticastDelegate<FMulticastScriptDelegate>, CASTCLASS_FMulticastInlineDelegateProperty)

public:

	FMulticastInlineDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_MulticastDelegate(InOwner, InName, InObjectFlags)
	{
	}

	FMulticastInlineDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
		: TProperty_MulticastDelegate(InOwner, InName, InObjectFlags, InOffset, InFlags, InSignatureFunction)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FMulticastInlineDelegateProperty(UField* InField)
		: TProperty_MulticastDelegate(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual const TCHAR* ImportText_Internal(const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText) const override;
	// End of FProperty interface

	// FMulticastDelegateProperty interface
	virtual const FMulticastScriptDelegate* GetMulticastDelegate(const void* PropertyValue) const override;
	virtual void SetMulticastDelegate(void* PropertyValue, FMulticastScriptDelegate ScriptDelegate) const override;

	virtual void AddDelegate(FScriptDelegate ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;
	virtual void RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;
	virtual void ClearDelegate(UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;

protected:
	virtual FMulticastScriptDelegate::FInvocationList& GetInvocationList(const void* PropertyValue) const;
	// End of FMulticastDelegateProperty interface
};

class COREUOBJECT_API FMulticastSparseDelegateProperty : public TProperty_MulticastDelegate<FSparseDelegate> 
{
	DECLARE_FIELD(FMulticastSparseDelegateProperty, TProperty_MulticastDelegate<FSparseDelegate>, CASTCLASS_FMulticastSparseDelegateProperty)

public:

	FMulticastSparseDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags)
		: TProperty_MulticastDelegate(InOwner, InName, InObjectFlags)
	{
	}

	FMulticastSparseDelegateProperty(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags, int32 InOffset, EPropertyFlags InFlags, UFunction* InSignatureFunction = nullptr)
		: TProperty_MulticastDelegate(InOwner, InName, InObjectFlags, InOffset, InFlags, InSignatureFunction)
	{
	}

#if WITH_EDITORONLY_DATA
	explicit FMulticastSparseDelegateProperty(UField* InField)
		: TProperty_MulticastDelegate(InField)
	{
	}
#endif // WITH_EDITORONLY_DATA

	// FProperty interface
	virtual void SerializeItem(FStructuredArchive::FSlot Slot, void* Value, void const* Defaults) const override;
	virtual const TCHAR* ImportText_Internal(const TCHAR* Buffer, void* Data, int32 PortFlags, UObject* OwnerObject, FOutputDevice* ErrorText) const override;
	// End of FProperty interface

	// FMulticastDelegateProperty interface
	virtual const FMulticastScriptDelegate* GetMulticastDelegate(const void* PropertyValue) const override;
	virtual void SetMulticastDelegate(void* PropertyValue, FMulticastScriptDelegate ScriptDelegate) const override;

	virtual void AddDelegate(FScriptDelegate ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;
	virtual void RemoveDelegate(const FScriptDelegate& ScriptDelegate, UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;
	virtual void ClearDelegate(UObject* Parent = nullptr, void* PropertyValue = nullptr) const override;

protected:
	virtual FMulticastScriptDelegate::FInvocationList& GetInvocationList(const void* PropertyValue) const;
	// End of FMulticastDelegateProperty interface

private:
	virtual void SerializeItemInternal(FArchive& Ar, void* Value, void const* Defaults) const;
};

/** Describes a single node in a custom property list. */
struct COREUOBJECT_API FCustomPropertyListNode
{
	/** The property that's being referenced at this node. */
	FProperty* Property;

	/** Used to identify which array index is specifically being referenced if this is an array property. Defaults to 0. */
	int32 ArrayIndex;

	/** If this node represents a struct property, this may contain a "sub" property list for the struct itself. */
	struct FCustomPropertyListNode* SubPropertyList;

	/** Points to the next node in the list. */
	struct FCustomPropertyListNode* PropertyListNext;

	/** Default constructor. */
	FCustomPropertyListNode(FProperty* InProperty = nullptr, int32 InArrayIndex = 0)
		:Property(InProperty)
		, ArrayIndex(InArrayIndex)
		, SubPropertyList(nullptr)
		, PropertyListNext(nullptr)
	{
	}

	/** Convenience method to return the next property in the list and advance the given ptr. */
	FORCEINLINE static FProperty* GetNextPropertyAndAdvance(const FCustomPropertyListNode*& Node)
	{
		if (Node)
		{
			Node = Node->PropertyListNext;
		}

		return Node ? Node->Property : nullptr;
	}
};


/**
 * This class represents the chain of member properties leading to an internal struct property.  It is used
 * for tracking which member property corresponds to the UScriptStruct that owns a particular property.
 */
class COREUOBJECT_API FEditPropertyChain : public TDoubleLinkedList<FProperty*>
{

public:
	/** Constructors */
	FEditPropertyChain() : ActivePropertyNode(NULL), ActiveMemberPropertyNode(NULL), bFilterAffectedInstances(false) {}

	/**
	 * Sets the ActivePropertyNode to the node associated with the property specified.
	 *
	 * @param	NewActiveProperty	the FProperty that is currently being evaluated by Pre/PostEditChange
	 *
	 * @return	true if the ActivePropertyNode was successfully changed to the node associated with the property
	 *			specified.  false if there was no node corresponding to that property.
	 */
	bool SetActivePropertyNode( FProperty* NewActiveProperty );

	/**
	 * Sets the ActiveMemberPropertyNode to the node associated with the property specified.
	 *
	 * @param	NewActiveMemberProperty		the member FProperty which contains the property currently being evaluated
	 *										by Pre/PostEditChange
	 *
	 * @return	true if the ActiveMemberPropertyNode was successfully changed to the node associated with the
	 *			property specified.  false if there was no node corresponding to that property.
	 */
	bool SetActiveMemberPropertyNode( FProperty* NewActiveMemberProperty );

	/**
	 * Specify the set of archetype instances that will be affected by the property change.
	 */
	template<typename T>
	void SetAffectedArchetypeInstances( T&& InAffectedInstances )
	{
		bFilterAffectedInstances = true;
		AffectedInstances = Forward<T>(InAffectedInstances);
	}
	
	/**
	 * Returns whether the specified archetype instance will be affected by the property change.
	 */
	bool IsArchetypeInstanceAffected( UObject* InInstance ) const;

	/**
	 * Returns the node corresponding to the currently active property.
	 */
	TDoubleLinkedListNode* GetActiveNode() const;

	/**
	 * Returns the node corresponding to the currently active property, or if the currently active property
	 * is not a member variable (i.e. inside of a struct/array), the node corresponding to the member variable
	 * which contains the currently active property.
	 */
	TDoubleLinkedListNode* GetActiveMemberNode() const;

protected:
	/**
	 * In a hierarchy of properties being edited, corresponds to the property that is currently
	 * being processed by Pre/PostEditChange
	 */
	TDoubleLinkedListNode* ActivePropertyNode;

	/**
	 * In a hierarchy of properties being edited, corresponds to the class member property which
	 * contains the property that is currently being processed by Pre/PostEditChange.  This will
	 * only be different from the ActivePropertyNode if the active property is contained within a struct,
	 * dynamic array, or static array.
	 */
	TDoubleLinkedListNode* ActiveMemberPropertyNode;

	/**
	 * Archetype instances that will be affected by the property change.
	 */
	TSet<UObject*> AffectedInstances;

	/**
	 * Assume all archetype instances are affected unless a set of affected instances is provided.
	 */
	bool bFilterAffectedInstances;

	/** TDoubleLinkedList interface */
	/**
	 * Updates the size reported by Num().  Child classes can use this function to conveniently
	 * hook into list additions/removals.
	 *
	 * This version ensures that the ActivePropertyNode and ActiveMemberPropertyNode point to a valid nodes or NULL if this list is empty.
	 *
	 * @param	NewListSize		the new size for this list
	 */
	virtual void SetListSize( int32 NewListSize );
};


//-----------------------------------------------------------------------------
//EPropertyNodeFlags - Flags used internally by property editors
//-----------------------------------------------------------------------------
namespace EPropertyChangeType
{
	typedef uint32 Type;

	//default value.  Add new enums to add new functionality.
	const Type Unspecified = 1 << 0;
	//Array Add
	const Type ArrayAdd = 1 << 1;
	//Array Remove
	const Type ArrayRemove = 1 << 2;
	//Array Clear
	const Type ArrayClear = 1 << 3;
	//Value Set
	const Type ValueSet = 1 << 4;
	//Duplicate
	const Type Duplicate = 1 << 5;
	//Interactive, e.g. dragging a slider. Will be followed by a ValueSet when finished.
	const Type Interactive = 1 << 6;
	//Redirected.  Used when property references are updated due to content hot-reloading, or an asset being replaced during asset deletion (aka, asset consolidation).
	const Type Redirected = 1 << 7;
};

/**
 * Structure for passing pre and post edit change events
 */
struct FPropertyChangedEvent
{
	FPropertyChangedEvent(FProperty* InProperty, EPropertyChangeType::Type InChangeType = EPropertyChangeType::Unspecified, TArrayView<const UObject* const> InTopLevelObjects = TArrayView<const UObject* const>())
		: Property(InProperty)
		, MemberProperty(InProperty)
		, ChangeType(InChangeType)
		, ObjectIteratorIndex(INDEX_NONE)
		, bFilterChangedInstances(false)
		, TopLevelObjects(InTopLevelObjects)
	{
	}

	UE_DEPRECATED(4.25, "The FPropertyChangedEvent constructor taking a TArray* is deprecated. Use the version taking a TArrayView instead.")
	FPropertyChangedEvent(FProperty* InProperty, EPropertyChangeType::Type InChangeType, const TArray<const UObject*>* InTopLevelObjects)
		: Property(InProperty)
		, MemberProperty(InProperty)
		, ChangeType(InChangeType)
		, ObjectIteratorIndex(INDEX_NONE)
		, bFilterChangedInstances(false)
	{
		if (InTopLevelObjects)
		{
			TopLevelObjects = MakeArrayView(*InTopLevelObjects);
		}
	}

	void SetActiveMemberProperty( FProperty* InActiveMemberProperty )
	{
		MemberProperty = InActiveMemberProperty;
	}

	/**
	 * Saves off map of array indices per object being set.
	 */
	void SetArrayIndexPerObject(TArrayView<const TMap<FString, int32>> InArrayIndices)
	{
		ArrayIndicesPerObject = InArrayIndices; 
	}

	/**
	 * Specify the set of archetype instances that were modified by the property change.
	 */
	template<typename T>
	void SetInstancesChanged(T&& InInstancesChanged)
	{
		bFilterChangedInstances = true;
		InstancesChanged = Forward<T>(InInstancesChanged);
	}

	/**
	 * Gets the Array Index of the "current object" based on a particular name
	 * InName - Name of the property to find the array index for
	 */
	int32 GetArrayIndex(const FString& InName)
	{
		//default to unknown index
		int32 Retval = -1;
		if (ArrayIndicesPerObject.IsValidIndex(ObjectIteratorIndex))
		{
			const int32* ValuePtr = ArrayIndicesPerObject[ObjectIteratorIndex].Find(InName);
			if (ValuePtr)
			{
				Retval = *ValuePtr;
			}
		}
		return Retval;
	}

	/**
	 * Test whether an archetype instance was modified.
	 * InInstance - The instance we want to know the status.
	 */
	bool HasArchetypeInstanceChanged(UObject* InInstance) const
	{
		return !bFilterChangedInstances || InstancesChanged.Contains(InInstance);
	}

	/**
	 * @return The number of objects being edited during this change event
	 */
	int32 GetNumObjectsBeingEdited() const { return TopLevelObjects.Num(); }

	/**
	 * Gets an object being edited by this change event.  Multiple objects could be edited at once
	 *
	 * @param Index	The index of the object being edited. Assumes index is valid.  Call GetNumObjectsBeingEdited first to check if there are valid objects
	 * @return The object being edited or nullptr if no object was found
	 */
	const UObject* GetObjectBeingEdited(int32 Index) const { return TopLevelObjects[Index]; }

	/**
	 * Simple utility to get the name of the property and takes care of the possible null property.
	 */
	FName GetPropertyName() const
	{
		return (Property != nullptr) ? Property->GetFName() : NAME_None;
	}

	/**
	 * The actual property that changed
	 */
	FProperty* Property;

	/**
	 * The member property of the object that PostEditChange is being called on.  
	 * For example if the property that changed is inside a struct on the object, this property is the struct property
	 */
	FProperty* MemberProperty;

	// The kind of change event that occurred
	EPropertyChangeType::Type ChangeType;

	// Used by the param system to say which object is receiving the event in the case of multi-select
	int32 ObjectIteratorIndex;
private:
	//In the property window, multiple objects can be selected at once.  In the case of adding/inserting to an array, each object COULD have different indices for the new entries in the array
	TArrayView<const TMap<FString, int32>> ArrayIndicesPerObject;
	
	//In the property window, multiple objects can be selected at once. In this case we want to know if an instance was updated for this operation (used in array/set/map context)
	TSet<UObject*> InstancesChanged;

	//Assume all archetype instances were changed unless a set of changed instances is provided.
	bool bFilterChangedInstances;

	/** List of top level objects being changed */
	TArrayView<const UObject* const> TopLevelObjects;
};

/**
 * Structure for passing pre and post edit change events
 */
struct FPropertyChangedChainEvent : public FPropertyChangedEvent
{
	FPropertyChangedChainEvent(FEditPropertyChain& InPropertyChain, FPropertyChangedEvent& SrcChangeEvent) :
		FPropertyChangedEvent(SrcChangeEvent),
		PropertyChain(InPropertyChain)
	{
	}
	FEditPropertyChain& PropertyChain;
};

/*-----------------------------------------------------------------------------
TFieldIterator.
-----------------------------------------------------------------------------*/

/** TFieldIterator construction flags */
enum class EFieldIterationFlags : uint8
{
	None = 0,
	IncludeSuper = 1<<0,		// Include super class
	IncludeDeprecated = 1<<1,	// Include deprecated properties
	IncludeInterfaces = 1<<2,	// Include interfaces

	IncludeAll = IncludeSuper | IncludeDeprecated | IncludeInterfaces,

	Default = IncludeSuper | IncludeDeprecated,
};
ENUM_CLASS_FLAGS(EFieldIterationFlags);

/** Old-style TFieldIterator construction flags */
namespace EFieldIteratorFlags
{
	enum SuperClassFlags
	{
		ExcludeSuper = (uint8)EFieldIterationFlags::None,
		IncludeSuper = (uint8)EFieldIterationFlags::IncludeSuper,
	};

	enum DeprecatedPropertyFlags
	{
		ExcludeDeprecated = (uint8)EFieldIterationFlags::None,
		IncludeDeprecated = (uint8)EFieldIterationFlags::IncludeDeprecated,
	};

	enum InterfaceClassFlags
	{
		ExcludeInterfaces = (uint8)EFieldIterationFlags::None,
		IncludeInterfaces = (uint8)EFieldIterationFlags::IncludeInterfaces,
	};
}

template <class FieldType>
FieldType* GetChildFieldsFromStruct(const UStruct* Owner)
{
	check(false);
	return nullptr;
}

template <>
inline UField* GetChildFieldsFromStruct(const UStruct* Owner)
{
	return Owner->Children;
}

template <>
inline FField* GetChildFieldsFromStruct(const UStruct* Owner)
{
	return Owner->ChildProperties;
}


//
// For iterating through a linked list of fields.
//
template <class T>
class TFieldIterator
{
private:
	/** The object being searched for the specified field */
	const UStruct* Struct;
	/** The current location in the list of fields being iterated */
	typename T::BaseFieldClass* Field;
	/** The index of the current interface being iterated */
	int32 InterfaceIndex;
	/** Whether to include the super class or not */
	const bool bIncludeSuper;
	/** Whether to include deprecated fields or not */
	const bool bIncludeDeprecated;
	/** Whether to include interface fields or not */
	const bool bIncludeInterface;

public:
	TFieldIterator(const UStruct* InStruct, EFieldIterationFlags InIterationFlags = EFieldIterationFlags::Default)
		: Struct            ( InStruct )
		, Field             ( InStruct ? GetChildFieldsFromStruct<typename T::BaseFieldClass>(InStruct) : NULL )
		, InterfaceIndex    ( -1 )
		, bIncludeSuper     ( EnumHasAnyFlags(InIterationFlags, EFieldIterationFlags::IncludeSuper) )
		, bIncludeDeprecated( EnumHasAnyFlags(InIterationFlags, EFieldIterationFlags::IncludeDeprecated) )
		, bIncludeInterface ( EnumHasAnyFlags(InIterationFlags, EFieldIterationFlags::IncludeInterfaces) && InStruct && InStruct->IsA(UClass::StaticClass()) )
	{
		IterateToNext();
	}

	/** Legacy version taking the flags as 3 separate values */
	TFieldIterator(const UStruct*                               InStruct,
	               EFieldIteratorFlags::SuperClassFlags         InSuperClassFlags,
	               EFieldIteratorFlags::DeprecatedPropertyFlags InDeprecatedFieldFlags = EFieldIteratorFlags::IncludeDeprecated,
	               EFieldIteratorFlags::InterfaceClassFlags     InInterfaceFieldFlags  = EFieldIteratorFlags::ExcludeInterfaces)
		: TFieldIterator(InStruct, (EFieldIterationFlags)(InSuperClassFlags | InDeprecatedFieldFlags | InInterfaceFieldFlags))
	{
	}

	/** conversion to "bool" returning true if the iterator is valid. */
	FORCEINLINE explicit operator bool() const
	{ 
		return Field != NULL; 
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const 
	{
		return !(bool)*this;
	}

	inline friend bool operator==(const TFieldIterator<T>& Lhs, const TFieldIterator<T>& Rhs) { return Lhs.Field == Rhs.Field; }
	inline friend bool operator!=(const TFieldIterator<T>& Lhs, const TFieldIterator<T>& Rhs) { return Lhs.Field != Rhs.Field; }

	inline void operator++()
	{
		checkSlow(Field);
		Field = Field->Next;
		IterateToNext();
	}
	inline T* operator*()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	inline const T* operator*() const
	{
		checkSlow(Field);
		return (const T*)Field;
	}
	inline T* operator->()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	inline const UStruct* GetStruct()
	{
		return Struct;
	}
protected:
	inline void IterateToNext()
	{
		typename T::BaseFieldClass* CurrentField  = Field;
		const UStruct* CurrentStruct = Struct;

		while (CurrentStruct)
		{
			while (CurrentField)
			{
				typename T::FieldTypeClass* FieldClass = CurrentField->GetClass();

				if (FieldClass->HasAllCastFlags(T::StaticClassCastFlags()) &&
					(
						   bIncludeDeprecated
						|| !FieldClass->HasAllCastFlags(CASTCLASS_FProperty)
						|| !((FProperty*)CurrentField)->HasAllPropertyFlags(CPF_Deprecated)
					)
				)
				{
					Struct = CurrentStruct;
					Field  = CurrentField;
					return;
				}

				CurrentField = CurrentField->Next;
			}

			if (bIncludeInterface)
			{
				// We shouldn't be able to get here for non-classes
				UClass* CurrentClass = (UClass*)CurrentStruct;
				++InterfaceIndex;
				if (InterfaceIndex < CurrentClass->Interfaces.Num())
				{
					FImplementedInterface& Interface = CurrentClass->Interfaces[InterfaceIndex];
					CurrentField = Interface.Class ? GetChildFieldsFromStruct<typename T::BaseFieldClass>(Interface.Class) : nullptr;
					continue;
				}
			}

			if (bIncludeSuper)
			{
				CurrentStruct = CurrentStruct->GetInheritanceSuper();
				if (CurrentStruct)
				{
					CurrentField   = GetChildFieldsFromStruct<typename T::BaseFieldClass>(CurrentStruct);
					InterfaceIndex = -1;
					continue;
				}
			}

			break;
		}

		Struct = CurrentStruct;
		Field  = CurrentField;
	}
};

template <typename T>
struct TFieldRange
{
	TFieldRange(const UStruct* InStruct, EFieldIterationFlags InIterationFlags = EFieldIterationFlags::Default)
		: Begin(InStruct, InIterationFlags)
	{
	}

	/** Legacy version taking the flags as 3 separate values */
	TFieldRange(const UStruct*                               InStruct,
	            EFieldIteratorFlags::SuperClassFlags         InSuperClassFlags,
	            EFieldIteratorFlags::DeprecatedPropertyFlags InDeprecatedFieldFlags = EFieldIteratorFlags::IncludeDeprecated,
	            EFieldIteratorFlags::InterfaceClassFlags     InInterfaceFieldFlags  = EFieldIteratorFlags::ExcludeInterfaces)
		: TFieldRange(InStruct, (EFieldIterationFlags)(InSuperClassFlags | InDeprecatedFieldFlags | InInterfaceFieldFlags))
	{
	}

	friend TFieldIterator<T> begin(const TFieldRange& Range) { return Range.Begin; }
	friend TFieldIterator<T> end  (const TFieldRange& Range) { return TFieldIterator<T>(NULL); }

	TFieldIterator<T> Begin;
};

/*-----------------------------------------------------------------------------
	Field templates.
-----------------------------------------------------------------------------*/

//
// Find a typed field in a struct.
//
template <class T>
UE_DEPRECATED(4.25, "FindField will no longer return properties. Use FindFProperty instead or FindUField if you want to find functions or enums.")
 T* FindField( const UStruct* Owner, FName FieldName )
{
	// We know that a "none" field won't exist in this Struct
	if( FieldName.IsNone() )
	{
		return nullptr;
	}

	// Search by comparing FNames (INTs), not strings
	for( TFieldIterator<T>It( Owner ); It; ++It )
	{
		if( It->GetFName() == FieldName )
		{
			return *It;
		}
	}

	// If we didn't find it, return no field
	return nullptr;
}

template <class T>
UE_DEPRECATED(4.25, "FindField will no longer return properties. Use FindFProperty instead or FindUField if you want to find UFunctions or UEnums.")
T* FindField( const UStruct* Owner, const TCHAR* FieldName )
{
	// lookup the string name in the Name hash
	FName Name(FieldName, FNAME_Find);
	return FindField<T>(Owner, Name);
}

template <class T> 
typename TEnableIf<TIsDerivedFrom<T, UField>::IsDerived, T*>::Type FindUField(const UStruct* Owner, FName FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	static_assert(sizeof(T) > 0, "T must not be an incomplete type");

	// We know that a "none" field won't exist in this Struct
	if (FieldName.IsNone())
	{
		return nullptr;
	}

	// Search by comparing FNames (INTs), not strings
	for (TFieldIterator<T>It(Owner, IterationFlags); It; ++It)
	{
		if (It->GetFName() == FieldName)
		{
			return *It;
		}
	}

	// If we didn't find it, return no field
	return nullptr;
}

template <class T> 
typename TEnableIf<TIsDerivedFrom<T, UField>::IsDerived, T*>::Type FindUField(const UStruct* Owner, const TCHAR* FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	static_assert(sizeof(T) > 0, "T must not be an incomplete type");

	// lookup the string name in the Name hash
	FName Name(FieldName, FNAME_Find);
	return FindUField<T>(Owner, Name, IterationFlags);
}

template <class T>
typename TEnableIf<TIsDerivedFrom<T, FField>::IsDerived, T*>::Type FindFProperty(const UStruct* Owner, FName FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	static_assert(sizeof(T) > 0, "T must not be an incomplete type");

	// We know that a "none" field won't exist in this Struct
	if (FieldName.IsNone())
	{
		return nullptr;
	}

	// Search by comparing FNames (INTs), not strings
	for (TFieldIterator<T>It(Owner, IterationFlags); It; ++It)
	{
		if (It->GetFName() == FieldName)
		{
			return *It;
		}
	}

	// If we didn't find it, return no field
	return nullptr;
}

template <class T>
typename TEnableIf<TIsDerivedFrom<T, FField>::IsDerived, T*>::Type FindFProperty(const UStruct* Owner, const TCHAR* FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	static_assert(sizeof(T) > 0, "T must not be an incomplete type");

	// lookup the string name in the Name hash
	FName Name(FieldName, FNAME_Find);
	return FindFProperty<T>(Owner, Name, IterationFlags);
}

/** Finds FProperties or UFunctions and UEnums */
inline FFieldVariant FindUFieldOrFProperty(const UStruct* Owner, FName FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	// Look for properties first as they're most often the runtime thing higher level code wants to find
	FFieldVariant Result = FindFProperty<FProperty>(Owner, FieldName, IterationFlags);
	if (!Result)
	{
		Result = FindUField<UField>(Owner, FieldName, IterationFlags);
	}
	return Result;
}

/** Finds FProperties or UFunctions and UEnums */
inline FFieldVariant FindUFieldOrFProperty(const UStruct* Owner, const TCHAR* FieldName, EFieldIterationFlags IterationFlags = EFieldIterationFlags::Default)
{
	// lookup the string name in the Name hash
	FName Name(FieldName, FNAME_Find);
	return FindUFieldOrFProperty(Owner, Name, IterationFlags);
}

/**
 * Search for the named field within the specified scope, including any Outer classes; assert on failure.
 *
 * @param	Scope		the scope to search for the field in
 * @param	FieldName	the name of the field to search for.
 */
template<typename T>
T* FindFieldChecked( const UStruct* Scope, FName FieldName )
{
	if ( FieldName != NAME_None && Scope != NULL )
	{
		const UStruct* InitialScope = Scope;
		for ( ; Scope != NULL; Scope = dynamic_cast<const UStruct*>(Scope->GetOuter()) )
		{
			for ( TFieldIterator<T> It(Scope); It; ++It )
			{
				if ( It->GetFName() == FieldName )
				{
					return *It;
				}
			}
		}
	
		UE_LOG( LogType, Fatal, TEXT("Failed to find %s %s in %s"), *T::StaticClass()->GetName(), *FieldName.ToString(), *InitialScope->GetFullName() );
	}

	return NULL;
}

/*-----------------------------------------------------------------------------*
 * PropertyValueIterator
 *-----------------------------------------------------------------------------*/

/** FPropertyValueIterator construction flags */
enum class EPropertyValueIteratorFlags : uint8
{
	NoRecursion = 0,	// Don't recurse at all, only do top level properties
	FullRecursion = 1,	// Recurse into containers and structs
};

/** For recursively iterating over a UStruct to find nested FProperty pointers and values */
class FPropertyValueIterator
{
public:
	typedef TPair<const FProperty*, const void*> BasePairType;

	/** 
	 * Construct an iterator using a struct and struct value
	 *
	 * @param InPropertyClass	The UClass of the FProperty type you are looking for
	 * @param InStruct			The UClass or UScriptStruct containing properties to search for
	 * @param InStructValue		Address in memory of struct to search for property values
	 * @param InRecursionFlags	Rather to recurse into container and struct properties
	 * @param InDeprecatedPropertyFlags	Rather to iterate over deprecated properties
	 */
	COREUOBJECT_API FPropertyValueIterator(FFieldClass* InPropertyClass, const UStruct* InStruct, const void* InStructValue,
		EPropertyValueIteratorFlags						InRecursionFlags = EPropertyValueIteratorFlags::FullRecursion,
		EFieldIteratorFlags::DeprecatedPropertyFlags	InDeprecatedPropertyFlags = EFieldIteratorFlags::IncludeDeprecated);

	/** Invalid iterator, start with empty stack */
	FPropertyValueIterator()
		: PropertyClass(nullptr)
		, RecursionFlags(EPropertyValueIteratorFlags::FullRecursion)
		, DeprecatedPropertyFlags(EFieldIteratorFlags::IncludeDeprecated)
		, bSkipRecursionOnce(false)
	{
	}

	/** Conversion to "bool" returning true if the iterator is valid */
	FORCEINLINE explicit operator bool() const
	{
		// If nothing left in the stack, iteration is complete
		if (PropertyIteratorStack.Num() > 0)
		{
			return true;
		}
		return false;
	}

	FORCEINLINE friend bool operator==(const FPropertyValueIterator& Lhs, const FPropertyValueIterator& Rhs) 
	{
		return Lhs.PropertyIteratorStack == Rhs.PropertyIteratorStack;
	}
	
	FORCEINLINE friend bool operator!=(const FPropertyValueIterator& Lhs, const FPropertyValueIterator& Rhs)
	{
		return !(Lhs.PropertyIteratorStack == Rhs.PropertyIteratorStack);
	}

	/** Returns a TPair containing Property/Value currently being iterated */
	FORCEINLINE const BasePairType& operator*() const
	{
		const FPropertyValueStackEntry& Entry = PropertyIteratorStack.Last();
		return Entry.GetPropertyValue();
	}

	FORCEINLINE const BasePairType* operator->() const
	{
		const FPropertyValueStackEntry& Entry = PropertyIteratorStack.Last();
		return &Entry.GetPropertyValue();
	}

	/** Returns Property currently being iterated */
	FORCEINLINE const FProperty* Key() const 
	{
		return (*this)->Key; 
	}
	
	/** Returns memory address currently being iterated */
	FORCEINLINE const void* Value() const
	{
		return (*this)->Value;
	}

	/** Increments iterator */
	FORCEINLINE void operator++()
	{
		IterateToNext();
	}

	/** Call when iterating a recursive property such as Array or Struct to stop it from iterating into that property */
	FORCEINLINE void SkipRecursiveProperty()
	{
		bSkipRecursionOnce = true;
	}

	/** 
	 * Returns the full stack of properties for the property currently being iterated. This includes struct and container properties
	 *
	 * @param PropertyChain	Filled in with ordered list of Properties, with currently active property first and top parent last
	 */
	COREUOBJECT_API void GetPropertyChain(TArray<const FProperty*>& PropertyChain) const;

private:
	enum EPropertyValueFlags : uint8
	{
		IsMatch = 0x01,

		ContainerMask = 0xF0,
		IsArray = 0x10,
		IsMap = 0x20,
		IsSet = 0x40,
		IsStruct = 0x80,
	};

	struct FPropertyValueStackEntry
	{
		/** Address of owning UStruct or FProperty container */
		const void* Owner = nullptr;
		
		/** List of current root property+value pairs for the current top level FProperty */
		typedef TPair<BasePairType, uint8> BasePairAndFlags;
		typedef TArray<BasePairAndFlags, TInlineAllocator<8>> FValueArrayType;
		FValueArrayType ValueArray;

		/** Current position inside ValueArray */
		int32 ValueIndex = -1;

		/** Next position inside ValueArray */
		int32 NextValueIndex = 0;

		FPropertyValueStackEntry(const void* InValue)
			: Owner(InValue)
		{}

		FPropertyValueStackEntry(const UStruct* InStruct, const void* InValue, EFieldIteratorFlags::DeprecatedPropertyFlags InDeprecatedPropertyFlags)
			: Owner(InValue)
		{}

		FORCEINLINE friend bool operator==(const FPropertyValueStackEntry& Lhs, const FPropertyValueStackEntry& Rhs)
		{
			return Lhs.Owner == Rhs.Owner && Lhs.ValueIndex == Rhs.ValueIndex;
		}

		FORCEINLINE const BasePairType& GetPropertyValue() const
		{
			// Index has to be valid to get this far
			return ValueArray[ValueIndex].Key;
		}
	};

	/** Internal stack, one per continer/struct */
	TArray<FPropertyValueStackEntry, TInlineAllocator<8>> PropertyIteratorStack;

	/** Property type that is explicitly checked for */
	FFieldClass* PropertyClass = nullptr;

	/** Whether to recurse into containers/structs */
	const EPropertyValueIteratorFlags RecursionFlags;

	/** Inherits to child field iterator */
	const EFieldIteratorFlags::DeprecatedPropertyFlags DeprecatedPropertyFlags;

	/** If true, next iteration will skip recursing into containers/structs */
	bool bSkipRecursionOnce = false;

	/** If true, all properties will be matched without checking IsA(PropertyClass) */
	bool bMatchAll = false;

	/** Returns EPropertyValueFlags to describe if this Property is a match and/or a container/struct */
	uint8 GetPropertyValueFlags(const FProperty* Property);

	/** Fills the Entry.ValueArray with all relevant properties found in Struct */
	void FillStructProperties(const UStruct* Struct, FPropertyValueStackEntry& Entry);

	/**
	 * Goes to the next Property/value pair.
	 * Returns false on a match or out of properties, true when iteration should continue
	 */
	bool NextValue(EPropertyValueIteratorFlags RecursionFlags);

	/** Iterates to next property being checked for or until reaching the end of the structure */
	COREUOBJECT_API void IterateToNext();
};

/** Templated version, will verify the property type is correct and will skip any properties that are not */
template <class T>
class TPropertyValueIterator : public FPropertyValueIterator
{
public:
	typedef TPair<T*, const void*> PairType;
	
	/** 
	 * Construct an iterator using a struct and struct value
	 *
	 * @param InStruct			The UClass or UScriptStruct containing properties to search for
	 * @param InStructValue		Address in memory of struct to search for property values
	 * @param InRecursionFlags	Rather to recurse into container and struct properties
	 * @param InDeprecatedPropertyFlags	Rather to iterate over deprecated properties
	 */
	TPropertyValueIterator(const UStruct* InStruct, const void* InStructValue,
		EPropertyValueIteratorFlags						InRecursionFlags = EPropertyValueIteratorFlags::FullRecursion,
		EFieldIteratorFlags::DeprecatedPropertyFlags	InDeprecatedPropertyFlags = EFieldIteratorFlags::IncludeDeprecated)
		: FPropertyValueIterator(T::StaticClass(), InStruct, InStructValue, InRecursionFlags, InDeprecatedPropertyFlags)
	{
	}

	/** Invalid iterator, start with empty stack */
	TPropertyValueIterator() 
		: FPropertyValueIterator()
	{
	}

	/** Returns a TPair containing Property/Value currently being iterated */
	FORCEINLINE const PairType& operator*() const
	{
		return (const PairType&)FPropertyValueIterator::operator*();
	}

	FORCEINLINE const PairType* operator->() const
	{
		return (const PairType*)FPropertyValueIterator::operator->();
	}

	/** Returns Property currently being iterated */
	FORCEINLINE T* Key() const
	{
		return (*this)->Key;
	}
};

/** Templated range to allow ranged-for syntax */
template <class T>
struct TPropertyValueRange
{
	/** 
	 * Construct a range using a struct and struct value
	 *
	 * @param InStruct			The UClass or UScriptStruct containing properties to search for
	 * @param InStructValue		Address in memory of struct to search for property values
	 * @param InRecursionFlags	Rather to recurse into container and struct properties
	 * @param InDeprecatedPropertyFlags	Rather to iterate over deprecated properties
	 */
	TPropertyValueRange(const UStruct* InStruct, const void* InStructValue,
		EPropertyValueIteratorFlags						InRecursionFlags = EPropertyValueIteratorFlags::FullRecursion,
		EFieldIteratorFlags::DeprecatedPropertyFlags	InDeprecatedPropertyFlags = EFieldIteratorFlags::IncludeDeprecated)
		: Begin(InStruct, InStructValue, InRecursionFlags, InDeprecatedPropertyFlags)
	{
	}

	friend TPropertyValueIterator<T> begin(const TPropertyValueRange& Range) { return Range.Begin; }
	friend TPropertyValueIterator<T> end(const TPropertyValueRange& Range) { return TPropertyValueIterator<T>(); }

	TPropertyValueIterator<T> Begin;
};

/**
 * Determine if this object has SomeObject in its archetype chain.
 */
inline bool UObject::IsBasedOnArchetype(  const UObject* const SomeObject ) const
{
	checkfSlow(this, TEXT("IsBasedOnArchetype() is called on a null pointer. Fix the call site."));
	if ( SomeObject != this )
	{
		for ( UObject* Template = GetArchetype(); Template; Template = Template->GetArchetype() )
		{
			if ( SomeObject == Template )
			{
				return true;
			}
		}
	}

	return false;
}


/*-----------------------------------------------------------------------------
	C++ property macros.
-----------------------------------------------------------------------------*/

static_assert(sizeof(bool) == sizeof(uint8), "Bool is not one byte.");

/** helper to calculate an array's dimensions **/
#define CPP_ARRAY_DIM(ArrayName, ClassName) \
	(sizeof(((ClassName*)0)->ArrayName) / sizeof(((ClassName*)0)->ArrayName[0]))


/**
 * FProperty wrapper object.
 * The purpose of this object is to provide a UObject wrapper for native FProperties that can
 * be used by property editors (grids).
 * Specialized wrappers can be used to allow specialized editors for specific property types.
 * Property wrappers are owned by UStruct that owns the property they wrap and are tied to its lifetime
 * so that weak object pointer functionality works as expected.
 */
class COREUOBJECT_API UPropertyWrapper : public UObject
{
	DECLARE_CLASS_INTRINSIC(UPropertyWrapper, UObject, CLASS_Transient, TEXT("/Script/CoreUObject"));

protected:
	/** Cached property object */
	FProperty* DestProperty;
public:
	/** Sets the property this object wraps */
	void SetProperty(FProperty* InProperty)
	{
		DestProperty = InProperty;
	}
	/* Gets property wrapped by this object */
	FProperty* GetProperty()
	{
		return DestProperty;
	}
	/* Gets property wrapped by this object */
	const FProperty* GetProperty() const
	{
		return DestProperty;
	}
};

class COREUOBJECT_API UMulticastDelegatePropertyWrapper : public UPropertyWrapper
{
	DECLARE_CLASS_INTRINSIC(UMulticastDelegatePropertyWrapper, UPropertyWrapper, CLASS_Transient, TEXT("/Script/CoreUObject"));
};

class COREUOBJECT_API UMulticastInlineDelegatePropertyWrapper : public UMulticastDelegatePropertyWrapper
{
	DECLARE_CLASS_INTRINSIC(UMulticastInlineDelegatePropertyWrapper, UMulticastDelegatePropertyWrapper, CLASS_Transient, TEXT("/Script/CoreUObject"));
};

#include "UObject/DefineUPropertyMacros.h"
