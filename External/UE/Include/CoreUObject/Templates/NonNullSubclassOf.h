// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "Templates/ChooseClass.h"
#include "SubclassOf.h"

// So we can construct uninitialized TNonNullSubclassOf
enum class EDefaultConstructNonNullSubclassOf { UnsafeDoNotUse };

/**
 * Template to allow TClassType's to be passed around with type safety 
 */
template<class TClass>
class TNonNullSubclassOf : public TSubclassOf<TClass>
{
public:
	
	using TClassType = typename TSubclassOf<TClass>::TClassType;
	using TBaseType = typename TSubclassOf<TClass>::TBaseType;

	/** Default Constructor, defaults to null */
	FORCEINLINE TNonNullSubclassOf(EDefaultConstructNonNullSubclassOf) :
		TSubclassOf<TClass>(nullptr)
	{}

	/** Constructor that takes a UClass and does a runtime check to make sure this is a compatible class */
	FORCEINLINE TNonNullSubclassOf(TClassType* From) :
		TSubclassOf<TClass>(From)
	{
		checkf(From, TEXT("Initializing TNonNullSubclassOf with null"));
	}

	/** Copy Constructor, will only compile if types are compatible */
	template <class TClassA, class = decltype(ImplicitConv<TClass*>((TClassA*)nullptr))>
	FORCEINLINE TNonNullSubclassOf(const TSubclassOf<TClassA>& From) :
		TSubclassOf<TClass>(From)
	{}

	/** Assignment operator, will only compile if types are compatible */
	template <class TClassA, class = decltype(ImplicitConv<TClass*>((TClassA*)nullptr))>
	FORCEINLINE TNonNullSubclassOf& operator=(const TSubclassOf<TClassA>& From)
	{
		checkf(*From, TEXT("Assigning null to TNonNullSubclassOf"));
		TSubclassOf<TClass>::operator=(From);
		return *this;
	}
	
	/** Assignment operator from UClass, the type is checked on get not on set */
	FORCEINLINE TNonNullSubclassOf& operator=(TClassType* From)
	{
		checkf(From, TEXT("Assigning null to TNonNullSubclassOf"));
		TSubclassOf<TClass>::operator=(From);
		return *this;
	}
};
