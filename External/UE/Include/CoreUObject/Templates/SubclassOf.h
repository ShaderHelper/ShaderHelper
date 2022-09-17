// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "Templates/ChooseClass.h"

#include <type_traits>

template <typename T>
struct TIsTSubclassOf
{
	enum { Value = false };
};

/**
 * Template to allow TClassType's to be passed around with type safety 
 */
template<class TClass>
class TSubclassOf
{
public:

	typedef typename TChooseClass<TIsDerivedFrom<TClass, FField>::IsDerived, FFieldClass, UClass>::Result TClassType;
	typedef typename TChooseClass<TIsDerivedFrom<TClass, FField>::IsDerived, FField, UObject>::Result TBaseType;

private:

	template <class TClassA>
	friend class TSubclassOf;

public:
	/** Default Constructor, defaults to null */
	FORCEINLINE TSubclassOf() :
		Class(nullptr)
	{
	}

	/** Constructor that takes a UClass and does a runtime check to make sure this is a compatible class */
	FORCEINLINE TSubclassOf(TClassType* From) :
		Class(From)
	{
	}

	/** Constructor that takes a UClass and does a runtime check to make sure this is a compatible class */
	template <
		typename U,
		std::enable_if_t<
			!TIsTSubclassOf<std::decay_t<U>>::Value,
			decltype(ImplicitConv<TClassType*>(std::declval<U>()))
		>* = nullptr
	>
	FORCEINLINE TSubclassOf(U&& From)
		: Class(From)
	{
	}

	/** Copy Constructor, will only compile if types are compatible */
	template <class TClassA, class = decltype(ImplicitConv<TClass*>((TClassA*)nullptr))>
	FORCEINLINE TSubclassOf(const TSubclassOf<TClassA>& From) :
		Class(*From)
	{
	}

	/** Assignment operator, will only compile if types are compatible */
	template <class TClassA, class = decltype(ImplicitConv<TClass*>((TClassA*)nullptr))>
	FORCEINLINE TSubclassOf& operator=(const TSubclassOf<TClassA>& From)
	{
		Class = *From;
		return *this;
	}
	
	/** Assignment operator from UClass, the type is checked on get not on set */
	FORCEINLINE TSubclassOf& operator=(TClassType* From)
	{
		Class = From;
		return *this;
	}
	
	/** Assignment operator from UClass, the type is checked on get not on set */
	template <
		typename U,
		std::enable_if_t<
			!TIsTSubclassOf<std::decay_t<U>>::Value,
			decltype(ImplicitConv<TClassType*>(std::declval<U>()))
		>* = nullptr
	>
	FORCEINLINE TSubclassOf& operator=(U&& From)
	{
		Class = From;
		return *this;
	}
	
	/** Dereference back into a UClass, does runtime type checking */
	FORCEINLINE TClassType* operator*() const
	{
		if (!Class || !Class->IsChildOf(TClass::StaticClass()))
		{
			return nullptr;
		}
		return Class;
	}
	
	/** Dereference back into a UClass */
	FORCEINLINE TClassType* Get() const
	{
		return **this;
	}

	/** Dereference back into a UClass */
	FORCEINLINE TClassType* operator->() const
	{
		return **this;
	}

	/** Implicit conversion to UClass */
	FORCEINLINE operator TClassType* () const
	{
		return **this;
	}

	/**
	 * Get the CDO if we are referencing a valid class
	 *
	 * @return the CDO, or null if class is null
	 */
	FORCEINLINE TClass* GetDefaultObject() const
	{
		TBaseType* Result = nullptr;
		if (Class)
		{
			Result = Class->GetDefaultObject();
			check(Result && Result->IsA(TClass::StaticClass()));
		}
		return (TClass*)Result;
	}

	friend FArchive& operator<<(FArchive& Ar, TSubclassOf& SubclassOf)
	{
		Ar << SubclassOf.Class;
		return Ar;
	}

	friend uint32 GetTypeHash(const TSubclassOf& SubclassOf)
	{
		return GetTypeHash(SubclassOf.Class);
	}

#if DO_CHECK
	// This is a DEVELOPMENT ONLY debugging function and should not be relied upon. Client
	// systems should never require unsafe access to the referenced UClass
	UClass* DebugAccessRawClassPtr() const
	{
		return Class;
	}
#endif

private:
	TClassType* Class;
};

template <typename T>
struct TIsTSubclassOf<TSubclassOf<T>>
{
	enum { Value = true };
};
