// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Field.h: Declares FField property system fundamentals
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Script.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Math/RandomStream.h"
#include "UObject/GarbageCollection.h"
#include "UObject/CoreNative.h"
#include "Templates/IsAbstract.h"
#include "Templates/IsEnum.h"
#include "Misc/Optional.h"
#include "Misc/EnumClassFlags.h"
#include "Misc/CoreMiscDefines.h"
#include "HAL/ThreadSafeCounter.h"

#include <type_traits>

class FProperty;
class FField;
class FFieldVariant;

/**
  * Object representing a type of an FField struct. 
  * Mimics a subset of UObject reflection functions.
  */
class COREUOBJECT_API FFieldClass
{
	UE_NONCOPYABLE(FFieldClass);

	/** Name of this field class */
	FName Name;
	/** Unique Id of this field class (for casting) */
	uint64 Id;
	/** Cast flags used for casting to other classes */
	uint64 CastFlags;
	/** Class flags */
	EClassFlags ClassFlags;
	/** Super of this class */
	FFieldClass* SuperClass;	
	/** Default instance of this class */
	FField* DefaultObject;
	/** Pointer to a function that can construct an instance of this class */
	FField* (*ConstructFn)(const FFieldVariant&, const FName&, EObjectFlags);
	/** Counter for generating runtime unique names */
	FThreadSafeCounter UnqiueNameIndexCounter;

	/** Creates a default object instance of this class */
	FField* ConstructDefaultObject();

public:

	/** Gets the list of all field classes in existance */
	static TArray<FFieldClass*>& GetAllFieldClasses();
	/** Gets a mapping of all field class names to the actuall class objects */
	static TMap<FName, FFieldClass*>& GetNameToFieldClassMap();

	explicit FFieldClass(const TCHAR* InCPPName, uint64 InId, uint64 InCastFlags, FFieldClass* InSuperClass, FField* (*ConstructFnPtr)(const FFieldVariant&, const FName&, EObjectFlags));
	~FFieldClass();

	inline FString GetName() const
	{
		return Name.ToString();
	}
	inline FName GetFName() const
	{
		return Name;
	}
	inline uint64 GetId() const
	{
		return Id;
	}
	inline uint64 GetCastFlags() const
	{
		return CastFlags;
	}
	inline bool HasAnyCastFlags(const uint64 InCastFlags) const
	{
		return !!(CastFlags & InCastFlags);
	}
	inline bool HasAllCastFlags(const uint64 InCastFlags) const
	{
		return (CastFlags & InCastFlags) == InCastFlags;
	}
	inline bool IsChildOf(const FFieldClass* InClass) const
	{
		return !!(CastFlags & InClass->GetId());
	}
	FString GetDescription() const;
	FText GetDisplayNameText() const;
	FField* Construct(const FFieldVariant& InOwner, const FName& InName, EObjectFlags InFlags = RF_NoFlags) const
	{
		return ConstructFn(InOwner, InName, InFlags);
	}

	FFieldClass* GetSuperClass()
	{
		return SuperClass;
	}

	FField* GetDefaultObject()
	{
		if (!DefaultObject)
		{
			DefaultObject = ConstructDefaultObject();
			check(DefaultObject);
		}
		return DefaultObject;
	}

	bool HasAnyClassFlags(EClassFlags FlagsToCheck) const
	{
		return EnumHasAnyFlags(ClassFlags, FlagsToCheck) != 0;
	}

	int32 GetNextUniqueNameIndex()
	{
		return UnqiueNameIndexCounter.Increment();
	}

	friend FArchive& operator << (FArchive& Ar, FFieldClass& InField)
	{
		check(false);
		return Ar;
	}
	COREUOBJECT_API friend FArchive& operator << (FArchive& Ar, FFieldClass*& InOutFieldClass);
};

#if !CHECK_PUREVIRTUALS
	#define DECLARE_FIELD_NEW_IMPLEMENTATION(TClass) \
		ThisClass* Mem = (ThisClass*)FMemory::Malloc(InSize); \
		new (Mem) TClass(EC_InternalUseOnlyConstructor, TClass::StaticClass()); \
		return Mem; 
#else
	#define DECLARE_FIELD_NEW_IMPLEMENTATION(TClass) \
			ThisClass* Mem = (ThisClass*)FMemory::Malloc(InSize); \
			return Mem; 
#endif

#define DECLARE_FIELD(TClass, TSuperClass, TStaticFlags) \
private: \
	TClass& operator=(TClass&&);   \
	TClass& operator=(const TClass&);   \
public: \
	typedef TSuperClass Super;\
	typedef TClass ThisClass;\
	TClass(EInternal InInernal, FFieldClass* InClass) \
		: Super(EC_InternalUseOnlyConstructor, InClass) \
	{ \
	} \
	static FFieldClass* StaticClass(); \
	static FField* Construct(const FFieldVariant& InOwner, const FName& InName, EObjectFlags InObjectFlags); \
	inline static uint64 StaticClassCastFlagsPrivate() \
	{ \
		return uint64(TStaticFlags); \
	} \
	inline static uint64 StaticClassCastFlags() \
	{ \
		return uint64(TStaticFlags) | Super::StaticClassCastFlags(); \
	} \
	inline void* operator new(const size_t InSize, void* InMem) \
	{ \
		return InMem; \
	} \
	inline void* operator new(const size_t InSize) \
	{ \
		DECLARE_FIELD_NEW_IMPLEMENTATION(TClass) \
	} \
	inline void operator delete(void* InMem) noexcept \
	{ \
		FMemory::Free(InMem); \
	} \
	friend FArchive &operator<<( FArchive& Ar, ThisClass*& Res ) \
	{ \
		return Ar << (FField*&)Res; \
	} \
	friend void operator<<(FStructuredArchive::FSlot InSlot, ThisClass*& Res) \
	{ \
		InSlot << (FField*&)Res; \
	}

#if !CHECK_PUREVIRTUALS
	#define IMPLEMENT_FIELD_CONSTRUCT_IMPLEMENTATION(TClass) \
		FField* Instance = new TClass(InOwner, InName, InFlags); \
		return Instance; 
#else
	#define IMPLEMENT_FIELD_CONSTRUCT_IMPLEMENTATION(TClass) \
		return nullptr;
#endif

#define IMPLEMENT_FIELD(TClass) \
FField* TClass::Construct(const FFieldVariant& InOwner, const FName& InName, EObjectFlags InFlags) \
{ \
	IMPLEMENT_FIELD_CONSTRUCT_IMPLEMENTATION(TClass) \
} \
FFieldClass* TClass::StaticClass() \
{ \
	static FFieldClass StaticFieldClass(TEXT(#TClass), TClass::StaticClassCastFlagsPrivate(), TClass::StaticClassCastFlags(), TClass::Super::StaticClass(), &TClass::Construct); \
	return &StaticFieldClass; \
} \

class FProperty;
class FField;
class UObject;
class FLinkerLoad;

/**
 * Special container that can hold either UObject or FField.
 * Exposes common interface of FFields and UObjects for easier transition from UProperties to FProperties.
 * DO NOT ABUSE. IDEALLY THIS SHOULD ONLY BE FFIELD INTERNAL STRUCTURE FOR HOLDING A POINTER TO THE OWNER OF AN FFIELD.
 */
class COREUOBJECT_API FFieldVariant
{
	union FFieldObjectUnion
	{
		FField* Field;
		UObject* Object;
	} Container;

	bool bIsUObject;

public:

	FFieldVariant()
		: bIsUObject(false)
	{
		Container.Field = nullptr;
	}

	FFieldVariant(const FField* InField)
		: bIsUObject(false)
	{
		Container.Field = const_cast<FField*>(InField);
	}

	template <
		typename T,
		decltype(ImplicitConv<const UObject*>(std::declval<T>()))* = nullptr
	>
	FFieldVariant(T&& InObject)
		: bIsUObject(true)
	{
		Container.Object = const_cast<UObject*>(ImplicitConv<const UObject*>(InObject));
	}

	FFieldVariant(TYPE_OF_NULLPTR)
		: FFieldVariant()
	{
	}

	inline bool IsUObject() const
	{
		return bIsUObject;
	}
	inline bool IsValid() const
	{
		return !!Container.Object;
	}
	bool IsValidLowLevel() const;
	inline operator bool() const
	{
		return IsValid();
	}
	bool IsA(const UClass* InClass) const;
	bool IsA(const FFieldClass* InClass) const;
	template <typename T>
	bool IsA() const
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		return IsA(T::StaticClass());
	}

	template <typename T>
	typename TEnableIf<TIsDerivedFrom<T, UObject>::IsDerived, T*>::Type Get() const
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		if (IsA(T::StaticClass()))
		{
			return static_cast<T*>(Container.Object);
		}
		return nullptr;
	}

	template <typename T>
	typename TEnableIf<!TIsDerivedFrom<T, UObject>::IsDerived, T*>::Type Get() const
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		if (IsA(T::StaticClass()))
		{
			return static_cast<T*>(Container.Field);
		}
		return nullptr;
	}
	UObject* ToUObject() const
	{
		if (bIsUObject)
		{
			return Container.Object;
		}
		else
		{
			return nullptr;
		}
	}
	FField* ToField() const
	{
		if (!bIsUObject)
		{
			return Container.Field;
		}
		else
		{
			return nullptr;
		}
	}
	/** FOR INTERNAL USE ONLY: Function that returns the owner as FField without checking if it's actually an FField */
	FORCEINLINE FField* ToFieldUnsafe() const
	{
		return Container.Field;
	}
	/** FOR INTERNAL USE ONLY: Function that returns the owner as UObject without checking if it's actually a UObject */
	FORCEINLINE UObject* ToUObjectUnsafe() const
	{
		return Container.Object;
	}

	void* GetRawPointer() const
	{
		return Container.Field;
	}
	FFieldVariant GetOwnerVariant() const;
	UClass* GetOwnerClass() const;
	FString GetFullName() const;
	FString GetPathName() const;
	FString GetName() const;
	FString GetClassName() const;
	FName GetFName() const;
	bool IsNative() const;
	UPackage* GetOutermost() const;

	bool operator==(const FFieldVariant& Other) const
	{
		return Container.Field == Other.Container.Field;
	}
	bool operator!=(const FFieldVariant& Other) const
	{
		return Container.Field != Other.Container.Field;
	}

#if WITH_EDITORONLY_DATA
	bool HasMetaData(const FName& Key) const;
#endif

	/** Support comparison functions that make this usable as a KeyValue for a TSet<> */
	friend uint32 GetTypeHash(const FFieldVariant& InFieldVariant)
	{
		return GetTypeHash(InFieldVariant.GetRawPointer());
	}

	COREUOBJECT_API friend FArchive& operator << (FArchive& Ar, FFieldVariant& InOutField);
};

/**
 * Base class of reflection data objects.
 */
class COREUOBJECT_API FField
{
	UE_NONCOPYABLE(FField);

	/** Pointer to the class object representing the type of this FField */
	FFieldClass* ClassPrivate;

public:
	typedef FField Super;
	typedef FField ThisClass;
	typedef FField BaseFieldClass;	
	typedef FFieldClass FieldTypeClass;

	static FFieldClass* StaticClass();

	inline static uint64 StaticClassCastFlagsPrivate()
	{
		return uint64(CASTCLASS_UField);
	}
	inline static uint64 StaticClassCastFlags()
	{
		return uint64(CASTCLASS_UField);
	}

	/** Owner of this field */
	FFieldVariant Owner;

	/** Next Field in the linked list */
	FField* Next;

	/** Name of this field */
	FName NamePrivate;

	/** Object flags */
	EObjectFlags FlagsPrivate;

	// Constructors.
	FField(EInternal InInernal, FFieldClass* InClass);
	FField(FFieldVariant InOwner, const FName& InName, EObjectFlags InObjectFlags);
#if WITH_EDITORONLY_DATA
	explicit FField(UField* InField);
#endif // WITH_EDITORONLY_DATA
	virtual ~FField();

	// Begin UObject interface: the following functions mimic UObject interface for easier transition from UProperties to FProperties
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual void GetPreloadDependencies(TArray<UObject*>& OutDeps);
	virtual void BeginDestroy();	
	virtual void AddReferencedObjects(FReferenceCollector& Collector);
	bool IsRooted() const;
	bool IsNative() const;
	bool IsValidLowLevel() const;
	bool IsIn(const UObject* InOwner) const;
	bool IsIn(const FField* InOwner) const;
	FLinkerLoad* GetLinker() const;	
	// End UObject interface

	// Begin UField interface.
	virtual void AddCppProperty(FProperty* Property);
	virtual void Bind();
	// End UField interface

	/** Constructs a new field given its class */
	static FField* Construct(const FFieldVariant& InOwner, const FName& InName, EObjectFlags InFlags);
	/** Constructs a new field given the name of its class */
	static FField* Construct(const FName& FieldTypeName, const FFieldVariant& InOwner, const FName& InName, EObjectFlags InFlags);

	/** Fixups after duplicating a Field */
	virtual void PostDuplicate(const FField& InField);

protected:
	/**
	* Set the object flags directly
	**/
	void SetFlagsTo(EObjectFlags NewFlags)
	{
		checkfSlow((NewFlags & ~RF_AllFlags) == 0, TEXT("%s flagged as 0x%x but is trying to set flags to RF_AllFlags"), *GetFName().ToString(), (int32)FlagsPrivate);
		FlagsPrivate = NewFlags;
	}
public:
	/**
	* Retrieve the object flags directly
	*
	* @return Flags for this object
	**/
	EObjectFlags GetFlags() const
	{
		checkfSlow((FlagsPrivate & ~RF_AllFlags) == 0, TEXT("%s flagged as RF_AllFlags"), *GetFName().ToString());
		return FlagsPrivate;
	}

	void SetFlags(EObjectFlags NewFlags)
	{
		checkSlow(!(NewFlags & (RF_MarkAsNative | RF_MarkAsRootSet))); // These flags can't be used outside of constructors / internal code
		SetFlagsTo(GetFlags() | NewFlags);
	}
	void ClearFlags(EObjectFlags NewFlags)
	{
		checkSlow(!(NewFlags & (RF_MarkAsNative | RF_MarkAsRootSet)) || NewFlags == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		SetFlagsTo(GetFlags() & ~NewFlags);
	}
	/**
	* Used to safely check whether any of the passed in flags are set.
	*
	* @param FlagsToCheck	Object flags to check for.
	* @return				true if any of the passed in flags are set, false otherwise  (including no flags passed in).
	*/
	bool HasAnyFlags(EObjectFlags FlagsToCheck) const
	{
		checkSlow(!(FlagsToCheck & (RF_MarkAsNative | RF_MarkAsRootSet)) || FlagsToCheck == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		return (GetFlags() & FlagsToCheck) != 0;
	}
	/**
	* Used to safely check whether all of the passed in flags are set.
	*
	* @param FlagsToCheck	Object flags to check for
	* @return true if all of the passed in flags are set (including no flags passed in), false otherwise
	*/
	bool HasAllFlags(EObjectFlags FlagsToCheck) const
	{
		checkSlow(!(FlagsToCheck & (RF_MarkAsNative | RF_MarkAsRootSet)) || FlagsToCheck == RF_AllFlags); // These flags can't be used outside of constructors / internal code
		return ((GetFlags() & FlagsToCheck) == FlagsToCheck);
	}

	inline FFieldClass* GetClass() const
	{
		return ClassPrivate;
	}
	inline uint64 GetCastFlags() const
	{
		return GetClass()->GetCastFlags();
	}

	inline bool IsA(const FFieldClass* FieldType) const
	{
		check(FieldType);
		return !!(GetCastFlags() & FieldType->GetId());
	}

	template<typename T>
	bool IsA() const
	{
		return !!(GetCastFlags() & T::StaticClassCastFlagsPrivate());
	}

	inline bool HasAnyCastFlags(const uint64 InCastFlags) const
	{
		return !!(GetCastFlags() & InCastFlags);
	}

	inline bool HasAllCastFlags(const uint64 InCastFlags) const
	{
		return (GetCastFlags() & InCastFlags) == InCastFlags;
	}

	void AppendName(FString& ResultString) const
	{
		GetFName().AppendString(ResultString);
	}

	/** Gets the owner container for this field */
	FFieldVariant GetOwnerVariant() const
	{
		return Owner;
	}

	/** Goes up the outer chain to look for a UObject. This function is used in GC so for performance reasons it has to be inlined */
	FORCEINLINE UObject* GetOwnerUObject() const
	{
		FFieldVariant TempOuter = Owner;
		while (!TempOuter.IsUObject() && TempOuter.IsValid())
		{
			// It's ok to use the 'Unsafe' variant of ToField here since we just checked IsUObject above
			TempOuter = TempOuter.ToFieldUnsafe()->Owner;
		}
		return TempOuter.ToUObject();
	}

	/** Internal function for quickly getting the owner of this object as UObject. FOR INTERNAL USE ONLY */
	FORCEINLINE UObject* InternalGetOwnerAsUObjectUnsafe() const
	{
		return Owner.ToUObjectUnsafe();
	}

	/** Goes up the outer chain to look for a UClass */
	UClass* GetOwnerClass() const;

	/** Goes up the outer chain to look for a UStruct */
	UStruct* GetOwnerStruct() const;

	/** Goes up the outer chain to look for a UField */
	UField* GetOwnerUField() const;

	/** Goes up the outer chain to look for the outermost package */
	UPackage* GetOutermost() const;

	/** Goes up the outer chain to look for the outer of the specified type */
	UObject* GetTypedOwner(UClass* Target) const;

	/** Goes up the outer chain to look for the outer of the specified type */
	FField* GetTypedOwner(FFieldClass* Target) const;

	template <typename T>
	T* GetOwner() const
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		return Owner.Get<T>();
	}

	template <typename T>
	FUNCTION_NON_NULL_RETURN_START
	T* GetOwnerChecked() const
	FUNCTION_NON_NULL_RETURN_END
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		T* Result = Owner.Get<T>();
		check(Result);
		return Result;
	}

	template <typename T>
	T* GetTypedOwner() const
	{
		static_assert(sizeof(T) > 0, "T must not be an incomplete type");
		return static_cast<T*>(GetTypedOwner(T::StaticClass()));
	}

	FORCEINLINE FName GetFName() const
	{
		if (this != nullptr)
		{
			return NamePrivate;
		}
		else
		{
			return NAME_None;
		}
	}

	FORCEINLINE FString GetName() const
	{
		if (this != nullptr)
		{
			return NamePrivate.ToString();
		}
		else
		{
			return TEXT("None");
		}
	}

	FORCEINLINE void GetName(FString& OutName) const
	{
		if (this != nullptr)
		{
			return NamePrivate.ToString(OutName);
		}
		else
		{
			OutName = TEXT("None");
		}
	}

	void Rename(const FName& NewName);

	FString GetPathName(const UObject* StopOuter = nullptr) const;
	void GetPathName(const UObject* StopOuter, FStringBuilderBase& ResultString) const;
	FString GetFullName() const;

	/**
	 * Returns a human readable string that was assigned to this field at creation.
	 * By default this is the same as GetName() but it can be overridden if that is an internal-only name.
	 * This name is consistent in editor/cooked builds, is not localized, and is useful for data import/export.
	 */
	FString GetAuthoredName() const;

	/** Returns an inner field by name if the field has any */
	virtual FField* GetInnerFieldByName(const FName& InName)
	{
		return nullptr;
	}

	/** Fills the provided array with all inner fields this field owns (recursively) */
	virtual void GetInnerFields(TArray<FField*>& OutFields)
	{
	}

#if WITH_EDITORONLY_DATA

private:
	/** Editor-only meta data map */
	TMap<FName, FString>* MetaDataMap;

public:

	/**
	* Walks up the chain of packages until it reaches the top level, which it ignores.
	*
	* @param	bStartWithOuter		whether to include this object's name in the returned string
	* @return	string containing the path name for this object, minus the outermost-package's name
	*/
	FString GetFullGroupName(bool bStartWithOuter) const;

	/**
	* Finds the localized display name or native display name as a fallback.
	*
	* @return The display name for this object.
	*/
	FText GetDisplayNameText() const;

	/**
	* Finds the localized tooltip or native tooltip as a fallback.
	*
	* @param bShortTooltip Look for a shorter version of the tooltip (falls back to the long tooltip if none was specified)
	*
	* @return The tooltip for this object.
	*/
	FText GetToolTipText(bool bShortTooltip = false) const;

	/**
	* Determines if the property has any metadata associated with the key
	*
	* @param Key The key to lookup in the metadata
	* @return true if there is a (possibly blank) value associated with this key
	*/
	bool HasMetaData(const TCHAR* Key) const { return FindMetaData(Key) != nullptr; }
	bool HasMetaData(const FName& Key) const { return FindMetaData(Key) != nullptr; }

	/**
	* Find the metadata value associated with the key
	*
	* @param Key The key to lookup in the metadata
	* @return The value associated with the key if it exists, null otherwise
	*/
	const FString* FindMetaData(const TCHAR* Key) const;
	const FString* FindMetaData(const FName& Key) const;

	/**
	* Find the metadata value associated with the key
	*
	* @param Key The key to lookup in the metadata
	* @return The value associated with the key
	*/
	const FString& GetMetaData(const TCHAR* Key) const;
	const FString& GetMetaData(const FName& Key) const;

	/**
	* Find the metadata value associated with the key and localization namespace and key
	*
	* @param Key						The key to lookup in the metadata
	* @param LocalizationNamespace		Namespace to lookup in the localization manager
	* @param LocalizationKey			Key to lookup in the localization manager
	* @return							Localized metadata if available, defaults to whatever is provided via GetMetaData
	*/
	const FText GetMetaDataText(const TCHAR* MetaDataKey, const FString LocalizationNamespace = FString(), const FString LocalizationKey = FString()) const;
	const FText GetMetaDataText(const FName& MetaDataKey, const FString LocalizationNamespace = FString(), const FString LocalizationKey = FString()) const;

	/**
	* Sets the metadata value associated with the key
	*
	* @param Key The key to lookup in the metadata
	* @return The value associated with the key
	*/
	void SetMetaData(const TCHAR* Key, const TCHAR* InValue);
	void SetMetaData(const FName& Key, const TCHAR* InValue);

	void SetMetaData(const TCHAR* Key, FString&& InValue);
	void SetMetaData(const FName& Key, FString&& InValue);

	/**
	* Find the metadata value associated with the key
	* and return bool
	* @param Key The key to lookup in the metadata
	* @return return true if the value was true (case insensitive)
	*/
	bool GetBoolMetaData(const TCHAR* Key) const
	{
		const FString& BoolString = GetMetaData(Key);
		// FString == operator does case insensitive comparison
		return (BoolString == "true");
	}
	bool GetBoolMetaData(const FName& Key) const
	{
		const FString& BoolString = GetMetaData(Key);
		// FString == operator does case insensitive comparison
		return (BoolString == "true");
	}

	/**
	* Find the metadata value associated with the key
	* and return int32
	* @param Key The key to lookup in the metadata
	* @return the int value stored in the metadata.
	*/
	int32 GetIntMetaData(const TCHAR* Key) const
	{
		const FString& INTString = GetMetaData(Key);
		int32 Value = FCString::Atoi(*INTString);
		return Value;
	}
	int32 GetIntMetaData(const FName& Key) const
	{
		const FString& INTString = GetMetaData(Key);
		int32 Value = FCString::Atoi(*INTString);
		return Value;
	}

	/**
	* Find the metadata value associated with the key
	* and return float
	* @param Key The key to lookup in the metadata
	* @return the float value stored in the metadata.
	*/
	float GetFloatMetaData(const TCHAR* Key) const
	{
		const FString& FLOATString = GetMetaData(Key);
		// FString == operator does case insensitive comparison
		float Value = FCString::Atof(*FLOATString);
		return Value;
	}
	float GetFloatMetaData(const FName& Key) const
	{
		const FString& FLOATString = GetMetaData(Key);
		// FString == operator does case insensitive comparison
		float Value = FCString::Atof(*FLOATString);
		return Value;
	}

	/**
	* Find the metadata value associated with the key
	* and return Class
	* @param Key The key to lookup in the metadata
	* @return the class value stored in the metadata.
	*/
	UClass* GetClassMetaData(const TCHAR* Key) const;
	UClass* GetClassMetaData(const FName& Key) const;

	/** Clear any metadata associated with the key */
	void RemoveMetaData(const TCHAR* Key);
	void RemoveMetaData(const FName& Key);

	/** Gets all metadata associated with this field */
	const TMap<FName, FString>* GetMetaDataMap() const;

	/** Copies all metadata from Source Field to Dest Field */
	static void CopyMetaData(const FField* InSourceField, FField* InDestField);

	/** Creates a new FField from existing UField */
	static FField* CreateFromUField(UField* InField);
	
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnConvertCustomUFieldToFField, FFieldClass*, UField*, FField*&);
	/** Gets a delegate to convert custom UField types to FFields */
	static FOnConvertCustomUFieldToFField& GetConvertCustomUFieldToFFieldDelegate();

#endif // WITH_EDITORONLY_DATA

	/** Duplicates an FField */
	static FField* Duplicate(const FField* InField, FFieldVariant DestOwner, const FName DestName = NAME_None, EObjectFlags FlagMask = RF_AllFlags, EInternalObjectFlags InternalFlagsMask = EInternalObjectFlags::AllFlags);

	/** Generates a name for a Field of a given type. Each generated name is unique in the current runtime */
	static FName GenerateFFieldName(FFieldVariant InOwner, FFieldClass* InClass);
};

// Support for casting between different FFIeld types

template<typename FieldType>
FORCEINLINE FieldType* CastField(FField* Src)
{
	return Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? static_cast<FieldType*>(Src) : nullptr;
}

template<typename FieldType>
FORCEINLINE const FieldType* CastField(const FField* Src)
{
	return Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? static_cast<const FieldType*>(Src) : nullptr;
}

template<typename FieldType>
FORCEINLINE FieldType* ExactCastField(FField* Src)
{
	return (Src && (Src->GetClass() == FieldType::StaticClass())) ? static_cast<FieldType*>(Src) : nullptr;
}

template<typename FieldType>
FORCEINLINE FieldType* ExactCastField(const FField* Src)
{
	return (Src && (Src->GetClass() == FieldType::StaticClass())) ? static_cast<const FieldType*>(Src) : nullptr;
}

template<typename FieldType>
FUNCTION_NON_NULL_RETURN_START
FORCEINLINE FieldType* CastFieldChecked(FField* Src)
FUNCTION_NON_NULL_RETURN_END
{
#if !DO_CHECK
	return (FieldType*)Src;
#else
	FieldType* CastResult = Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? (FieldType*)Src : nullptr;
	checkf(CastResult, TEXT("CastFieldChecked failed with 0x%016llx"), (int64)(PTRINT)Src);
	return CastResult;
#endif // !DO_CHECK
}

template<typename FieldType>
FUNCTION_NON_NULL_RETURN_START
FORCEINLINE const FieldType* CastFieldChecked(const FField* Src)
FUNCTION_NON_NULL_RETURN_END
{
#if !DO_CHECK
	return (const FieldType*)Src;
#else
	const FieldType* CastResult = Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? (const FieldType*)Src : nullptr;
	checkf(CastResult, TEXT("CastFieldChecked failed with 0x%016llx"), (int64)(PTRINT)Src);
	return CastResult;
#endif // !DO_CHECK
}

template<typename FieldType>
FORCEINLINE FieldType* CastFieldCheckedNullAllowed(FField* Src)
{
#if !DO_CHECK
	return (FieldType*)Src;
#else
	FieldType* CastResult = Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? (FieldType*)Src : nullptr;
	checkf(CastResult || !Src, TEXT("CastFieldCheckedNullAllowed failed with 0x%016llx"), (int64)(PTRINT)Src);
	return CastResult;
#endif // !DO_CHECK
}

template<typename FieldType>
FORCEINLINE const FieldType* CastFieldCheckedNullAllowed(const FField* Src)
{
#if !DO_CHECK
	return (const FieldType*)Src;
#else
	const FieldType* CastResult = Src && Src->HasAnyCastFlags(FieldType::StaticClassCastFlagsPrivate()) ? (const FieldType*)Src : nullptr;
	checkf(CastResult || !Src, TEXT("CastFieldCheckedNullAllowed failed with 0x%016llx"), (int64)(PTRINT)Src);
	return CastResult;
#endif // !DO_CHECK
}

/**
 * Helper function for serializing FField to an archive. This function fully serializes the field and its properties.
 */
template <typename FieldType>
inline void SerializeSingleField(FArchive& Ar, FieldType*& Field, FFieldVariant Owner)
{
	if (Ar.IsLoading())
	{
		FName PropertyTypeName;
		Ar << PropertyTypeName;
		if (PropertyTypeName != NAME_None)
		{
			Field = CastField<FieldType>(FField::Construct(PropertyTypeName, Owner, NAME_None, RF_NoFlags));
			check(Field);
			Field->Serialize(Ar);
		}
		else
		{
			Field = nullptr;
		}
	}
	else
	{
		FName PropertyTypeName = Field ? Field->GetClass()->GetFName() : NAME_None;		
		Ar << PropertyTypeName;
		if (Field)
		{
			Field->Serialize(Ar);
		}
	}
}

/**
 * Gets the name of the provided field. If the field pointer is null, the result is "none"
 */
inline FString GetNameSafe(const FField* InField)
{
	if (InField)
	{
		return InField->GetName();
	}
	else
	{
		return TEXT("none");
	}
}
/**
 * Gets the full name of the provided field. If the field pointer is null, the result is "none"
 */
COREUOBJECT_API FString GetFullNameSafe(const FField* InField);
/**
 * Gets the path name of the provided field. If the field pointer is null, the result is "none"
 */
COREUOBJECT_API FString GetPathNameSafe(const FField* InField);
/** 
 * Finds a field given a path to the field (Package.Class[:Subobject:...]:FieldName)
 */
COREUOBJECT_API FField* FindFPropertyByPath(const TCHAR* InFieldPath);
/**
 * Templated version of FindFieldByPath
 */
template <typename FieldType>
inline FieldType* FindFProperty(const TCHAR* InFieldPath)
{
	FField* FoundField = FindFPropertyByPath(InFieldPath);
	return CastField<FieldType>(FoundField);
}
