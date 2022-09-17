// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Templates/Casts.h"
#include "ScriptInterface.h"

#include <type_traits>

/**
 * An alternative to TWeakObjectPtr that makes it easier to work through an interface.
 */
template<class T>
struct TWeakInterfacePtr
{
	/**
	 * Construct a new default weak pointer, pointing to null object.
	 */
	TWeakInterfacePtr() 
		: InterfaceInstance(nullptr) 
	{
	}

	/**
	 * Construct from an object pointer
	 * @param Object The object to create a weak pointer to. This object must implement interface T.
	 */
	template<
		typename U,
		decltype(ImplicitConv<typename TCopyQualifiersFromTo<U, UObject>::Type*>(std::declval<U>()))* = nullptr
	>
	TWeakInterfacePtr(U&& Object)
	{
		InterfaceInstance = Cast<T>(ImplicitConv<typename TCopyQualifiersFromTo<U, UObject>::Type*>(Object));
		if (InterfaceInstance != nullptr)
		{
			ObjectInstance = Object;
		}
	}

	/**
	 * Construct from an interface pointer
	 * @param Interface The interface pointer to create a weak pointer to. There must be a UObject behind the interface.
	 */
	TWeakInterfacePtr(T* Interface)
		: InterfaceInstance(nullptr)
	{
		ObjectInstance = Cast<UObject>(Interface);
		if (ObjectInstance != nullptr)
		{
			InterfaceInstance = Interface;
		}
	}

	UE_DEPRECATED(4.27, "Please use the constructor that takes a pointer")
	TWeakInterfacePtr(T& Interface)
		: InterfaceInstance(nullptr)
	{
		ObjectInstance = Cast<UObject>(&Interface);
		if (ObjectInstance != nullptr)
		{
			InterfaceInstance = &Interface;
		}
	}

	/**
	 * Reset the weak pointer back to the null state.
	 */
	FORCEINLINE void Reset()
	{
		InterfaceInstance = nullptr;
		ObjectInstance.Reset();
	}

	/**
	 * Test if this points to a live object. Parameters are passed to the underlying TWeakObjectPtr.
	 */
	FORCEINLINE bool IsValid(bool bEvenIfPendingKill, bool bThreadsafeTest = false) const
	{
		return InterfaceInstance != nullptr && ObjectInstance.IsValid(bEvenIfPendingKill, bThreadsafeTest);
	}

	/**
	 * Test if this points to a live object. Calls the underlying TWeakObjectPtr's parameterless IsValid method.
	 */
	FORCEINLINE bool IsValid() const
	{
		return InterfaceInstance != nullptr && ObjectInstance.IsValid();
	}

	/**
	 * Test if this pointer is stale. Parameters are passed to the underlying TWeakObjectPtr.
	 */
	FORCEINLINE bool IsStale(bool bEvenIfPendingKill = false, bool bThreadsafeTest = false) const
	{
		return InterfaceInstance != nullptr && ObjectInstance.IsStale(bEvenIfPendingKill, bThreadsafeTest);
	}

	/**
	 * Dereference the weak pointer into an interface pointer.
	 */
	FORCEINLINE T* Get() const
	{
		return IsValid() ? InterfaceInstance : nullptr;
	}

	/**
	 * Dereference the weak pointer into a UObject pointer.
	 */
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectInstance.Get();
	}

	/**
	 * Dereference the weak pointer.
	 */
	FORCEINLINE T& operator*() const
	{
		check(IsValid());
		return *InterfaceInstance;
	}

	/**
	 * Dereference the weak pointer.
	 */
	FORCEINLINE T* operator->() const
	{
		check(IsValid());
		return InterfaceInstance;
	}

	/**
	 * Assign from an interface pointer.
	 */
	FORCEINLINE TWeakInterfacePtr<T>& operator=(T* Other)
	{
		*this = TWeakInterfacePtr<T>(Other);
		return *this;
	}

	/**
	 * Assign from another weak pointer.
	 */
	FORCEINLINE TWeakInterfacePtr<T>& operator=(const TWeakInterfacePtr<T>& Other)
	{
		ObjectInstance = Other.ObjectInstance;
		InterfaceInstance = Other.InterfaceInstance;
		return *this;
	}

	/**
	 * Assign from a script interface.
	 */
	FORCEINLINE TWeakInterfacePtr<T>& operator=(const TScriptInterface<T>& Other)
	{
		ObjectInstance = Other.GetObject();
		InterfaceInstance = (T*)Other.GetInterface();
		return *this;
	}

	FORCEINLINE bool operator==(const TWeakInterfacePtr<T>& Other) const
	{
		return InterfaceInstance == Other.InterfaceInstance;
	}

	FORCEINLINE bool operator!=(const TWeakInterfacePtr<T>& Other) const
	{
		return InterfaceInstance != Other.InterfaceInstance;
	}

	UE_DEPRECATED(4.27, "Implicit equality with a UObject pointer has been deprecated - use GetObject() and test equality on its return value")
	FORCEINLINE bool operator==(const UObject* Other) const
	{
		return Other == ObjectInstance.Get();
	}

	FORCEINLINE TScriptInterface<T> ToScriptInterface() const
	{
		UObject* Object = ObjectInstance.Get();
		if (Object)
		{
			return TScriptInterface<T>(Object);
		}

		return TScriptInterface<T>();
	}

private:
	TWeakObjectPtr<UObject> ObjectInstance;
	T* InterfaceInstance;
};
