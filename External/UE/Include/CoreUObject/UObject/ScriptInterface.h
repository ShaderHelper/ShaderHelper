// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScriptInterface.h: Script interface definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "Templates/Casts.h"
#include "Templates/UnrealTemplate.h"

/**
 * FScriptInterface
 *
 * This utility class stores the FProperty data for a native interface property.  ObjectPointer and InterfacePointer point to different locations in the same UObject.
 */
class FScriptInterface
{
private:
	/**
	 * A pointer to a UObject that implements a native interface.
	 */
	UObject*	ObjectPointer = nullptr;

	/**
	 * Pointer to the location of the interface object within the UObject referenced by ObjectPointer.
	 */
	void*		InterfacePointer = nullptr;

	/**
	 * Serialize ScriptInterface
	 */
	FArchive& Serialize(FArchive& Ar, class UClass* InterfaceType);

public:
	/**
	 * Default constructor
	 */
	FScriptInterface() = default;

	/**
	 * Construction from object and interface
	 */
	FScriptInterface( UObject* InObjectPointer, void* InInterfacePointer )
	: ObjectPointer(InObjectPointer), InterfacePointer(InInterfacePointer)
	{}

	/**
	 * Copyable
	 */
	FScriptInterface(const FScriptInterface&) = default;
	FScriptInterface& operator=(const FScriptInterface&) = default;

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectPointer;
	}

	/**
	 * Returns the ObjectPointer contained by this FScriptInterface
	 */
	FORCEINLINE UObject*& GetObjectRef()
	{
		return ObjectPointer;
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE void* GetInterface() const
	{
		// only allow access to InterfacePointer if we have a valid ObjectPointer.  This is necessary because the garbage collector will set ObjectPointer to NULL
		// without using the accessor methods
		return ObjectPointer ? InterfacePointer : nullptr;
	}

	/**
	 * Sets the value of the ObjectPointer for this FScriptInterface
	 */
	FORCEINLINE void SetObject( UObject* InObjectPointer )
	{
		ObjectPointer = InObjectPointer;
		if ( ObjectPointer == nullptr )
		{
			SetInterface(nullptr);
		}
	}

	/**
	 * Sets the value of the InterfacePointer for this FScriptInterface
	 */
	FORCEINLINE void SetInterface( void* InInterfacePointer )
	{
		InterfacePointer = InInterfacePointer;
	}

	/**
	 * Comparison operator, taking a reference to another FScriptInterface
	 */
	FORCEINLINE bool operator==( const FScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && ObjectPointer == Other.GetObject();
	}
	FORCEINLINE bool operator!=( const FScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || ObjectPointer != Other.GetObject();
	}

	void AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(ObjectPointer);
	}

	friend inline uint32 GetTypeHash(const FScriptInterface& Instance)
	{
		return GetTypeHash(Instance.InterfacePointer);
	}
};



template<> struct TIsPODType<class FScriptInterface> { enum { Value = true }; };
template<> struct TIsZeroConstructType<class FScriptInterface> { enum { Value = true }; };

/**
 * Templated version of FScriptInterface, which provides accessors and operators for referencing the interface portion of a UObject that implements a native interface.
 */
template <typename InterfaceType>
class TScriptInterface : public FScriptInterface
{
public:
	/**
	 * Default constructor
	 */
	TScriptInterface() = default;

	/**
	 * Construction from nullptr
	 */
	TScriptInterface(TYPE_OF_NULLPTR) {}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <typename ObjectType, typename = decltype(ImplicitConv<UObject*>((ObjectType*)nullptr))>
	TScriptInterface(ObjectType* SourceObject)
	{
		SetObject(SourceObject);

		InterfaceType* SourceInterface = Cast<InterfaceType>(SourceObject);
		SetInterface(SourceInterface);
	}

	/**
	 * Copyable
	 */
	TScriptInterface(const TScriptInterface&) = default;
	TScriptInterface& operator=(const TScriptInterface&) = default;

	/**
	 * Assignment from nullptr
	 */
	TScriptInterface& operator=(TYPE_OF_NULLPTR)
	{
		*this = TScriptInterface();
		return *this;
	}

	/**
	 * Assignment from an object type that implements the InterfaceType native interface class
	 */
	template <typename ObjectType, typename = decltype(ImplicitConv<UObject*>((ObjectType*)nullptr))>
	TScriptInterface& operator=(ObjectType* SourceObject)
	{
		*this = TScriptInterface(SourceObject);
		return *this;
	}

	/**
	 * Comparison operator, taking a pointer to InterfaceType
	 */
	FORCEINLINE bool operator==( const InterfaceType* Other ) const
	{
		return GetInterface() == Other;
	}
	FORCEINLINE bool operator!=( const InterfaceType* Other ) const
	{
		return GetInterface() != Other;
	}

	/**
	 * Comparison operator, taking a reference to another TScriptInterface
	 */
	FORCEINLINE bool operator==( const TScriptInterface& Other ) const
	{
		return GetInterface() == Other.GetInterface() && GetObject() == Other.GetObject();
	}
	FORCEINLINE bool operator!=( const TScriptInterface& Other ) const
	{
		return GetInterface() != Other.GetInterface() || GetObject() != Other.GetObject();
	}

	/**
	 * Member access operator.  Provides transparent access to the interface pointer contained by this TScriptInterface
	 */
	FORCEINLINE InterfaceType* operator->() const
	{
		return GetInterface();
	}

	/**
	 * Dereference operator.  Provides transparent access to the interface pointer contained by this TScriptInterface
	 *
	 * @return	a reference (of type InterfaceType) to the object pointed to by InterfacePointer
	 */
	FORCEINLINE InterfaceType& operator*() const
	{
		return *GetInterface();
	}

	/**
	 * Returns the pointer to the interface
	 */
	FORCEINLINE InterfaceType* GetInterface() const
	{
		return (InterfaceType*)FScriptInterface::GetInterface();
	}

	/**
	 * Sets the value of the InterfacePointer for this TScriptInterface
	 */
	FORCEINLINE void SetInterface(InterfaceType* InInterfacePointer)
	{
		FScriptInterface::SetInterface(InInterfacePointer);
	}

	/**
	 * Boolean operator.  Provides transparent access to the interface pointer contained by this TScriptInterface.
	 *
	 * @return	true if InterfacePointer is non-NULL.
	 */
	FORCEINLINE explicit operator bool() const
	{
		return GetInterface() != nullptr;
	}

	friend FArchive& operator<<( FArchive& Ar, TScriptInterface& Interface )
	{
		return Interface.Serialize(Ar, InterfaceType::UClassType::StaticClass());
	}
};
