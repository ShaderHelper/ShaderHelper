// Copyright Epic Games, Inc. All Rights Reserved.

/*================================================================================
	DelegateInstancesImpl.inl: Inline implementation of delegate bindings.

	The types declared in this file are for internal use only. 
================================================================================*/

#pragma once
#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Templates/AreTypesEqual.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/RemoveReference.h"
#include "Templates/Tuple.h"
#include "Delegates/DelegateInstanceInterface.h"
#include "UObject/NameTypes.h"

class FDelegateBase;
class FDelegateHandle;
enum class ESPMode : uint8;

namespace UE4Delegates_Private
{
	constexpr bool IsUObjectPtr(const volatile UObjectBase*) { return true; }
	constexpr bool IsUObjectPtr(...)                         { return false; }
}

template <typename FuncType, typename UserPolicy, typename... VarTypes>
class TCommonDelegateInstanceState;

template <typename InRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TCommonDelegateInstanceState<InRetValType(ParamTypes...), UserPolicy, VarTypes...> : IBaseDelegateInstance<InRetValType(ParamTypes...), UserPolicy>
{
public:
	using RetValType = InRetValType;

public:
	explicit TCommonDelegateInstanceState(VarTypes... Vars)
		: Payload(Vars...)
		, Handle (FDelegateHandle::GenerateNewHandle)
	{
	}

	FDelegateHandle GetHandle() const final
	{
		return Handle;
	}

protected:
	// Payload member variables (if any).
	TTuple<VarTypes...> Payload;

	// The handle of this delegate
	FDelegateHandle Handle;
};

/**
 * Implements a delegate binding for UFunctions.
 *
 * @params UserClass Must be an UObject derived class.
 */
template <class UserClass, typename FuncType, typename UserPolicy, typename... VarTypes>
class TBaseUFunctionDelegateInstance;

template <class UserClass, typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TBaseUFunctionDelegateInstance<UserClass, WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseUFunctionDelegateInstance<UserClass, RetValType(ParamTypes...), UserPolicy, VarTypes...>;

	static_assert(UE4Delegates_Private::IsUObjectPtr((UserClass*)nullptr), "You cannot use UFunction delegates with non UObject classes.");

public:
	TBaseUFunctionDelegateInstance(UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
		: Super        (Vars...)
		, FunctionName (InFunctionName)
		, UserObjectPtr(InUserObject)
	{
		check(InFunctionName != NAME_None);

		if (InUserObject != nullptr)
		{
			CachedFunction = UserObjectPtr->FindFunctionChecked(InFunctionName);
		}
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return FunctionName;
	}

#endif

	UObject* GetUObject() const final
	{
		return (UObject*)UserObjectPtr.Get();
	}

	const void* GetObjectForTimerManager() const final
	{
		return UserObjectPtr.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
		return 0;
	}

	// Deprecated
	bool HasSameObject(const void* InUserObject) const final
	{
		return UserObjectPtr.Get() == InUserObject;
	}

	bool IsCompactable() const final
	{
		return !UserObjectPtr.Get(true);
	}

	bool IsSafeToExecute() const final
	{
		return UserObjectPtr.IsValid();
	}

public:

	// IBaseDelegateInstance interface

	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		using FParmsWithPayload = TPayload<RetValType(typename TDecay<ParamTypes>::Type..., typename TDecay<VarTypes> ::Type...)>;

		checkSlow(IsSafeToExecute());

		TPlacementNewer<FParmsWithPayload> PayloadAndParams;
		this->Payload.ApplyAfter(PayloadAndParams, Params...);
		UserObjectPtr->ProcessEvent(CachedFunction, &PayloadAndParams);
		return PayloadAndParams->GetResult();
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		if (UserClass* ActualUserObject = this->UserObjectPtr.Get())
		{
			using FParmsWithPayload = TPayload<RetValType(typename TDecay<ParamTypes>::Type..., typename TDecay<VarTypes> ::Type...)>;

			TPlacementNewer<FParmsWithPayload> PayloadAndParams;
			this->Payload.ApplyAfter(PayloadAndParams, Params...);
			ActualUserObject->ProcessEvent(CachedFunction, &PayloadAndParams);
			return true;
		}

		return false;
	}

public:

	/**
	 * Creates a new UFunction delegate binding for the given user object and function name.
	 *
	 * @param InObject The user object to call the function on.
	 * @param InFunctionName The name of the function call.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InUserObject, InFunctionName, Vars...);
	}

public:

	// Holds the cached UFunction to call.
	UFunction* CachedFunction;

	// Holds the name of the function to call.
	FName FunctionName;

	// The user object to call the function on.
	TWeakObjectPtr<UserClass> UserObjectPtr;
};


/* Delegate binding types
 *****************************************************************************/

/**
 * Implements a delegate binding for shared pointer member functions.
 */
template <bool bConst, class UserClass, ESPMode SPMode, typename FuncType, typename UserPolicy, typename... VarTypes>
class TBaseSPMethodDelegateInstance;

template <bool bConst, class UserClass, ESPMode SPMode, typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TBaseSPMethodDelegateInstance<bConst, UserClass, SPMode, WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseSPMethodDelegateInstance<bConst, UserClass, SPMode, RetValType(ParamTypes...), UserPolicy, VarTypes...>;

public:
	using FMethodPtr = typename TMemFunPtrType<bConst, UserClass, RetValType(ParamTypes..., VarTypes...)>::Type;

	TBaseSPMethodDelegateInstance(const TSharedPtr<UserClass, SPMode>& InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: Super     (Vars...)
		, UserObject(InUserObject)
		, MethodPtr (InMethodPtr)
	{
		// NOTE: Shared pointer delegates are allowed to have a null incoming object pointer.  Weak pointers can expire,
		//       an it is possible for a copy of a delegate instance to end up with a null pointer.
		checkSlow(MethodPtr != nullptr);
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const final
	{
		return UserObject.Pin().Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
#if PLATFORM_64BITS
		return *((uint64*)&MethodPtr);
#else
		return *((uint32*)&MethodPtr);
#endif
	}

	// Deprecated
	bool HasSameObject(const void* InUserObject) const final
	{
		return UserObject.HasSameObject(InUserObject);
	}

	bool IsSafeToExecute() const final
	{
		return UserObject.IsValid();
	}

public:

	// IBaseDelegateInstance interface

	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		using MutableUserClass = typename TRemoveConst<UserClass>::Type;

		// Verify that the user object is still valid.  We only have a weak reference to it.
		TSharedPtr<UserClass, SPMode> SharedUserObject = UserObject.Pin();
		checkSlow(SharedUserObject.IsValid());

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(SharedUserObject.Get());

		checkSlow(MethodPtr != nullptr);

		return this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		// Verify that the user object is still valid.  We only have a weak reference to it.
		if (TSharedPtr<UserClass, SPMode> SharedUserObject = this->UserObject.Pin())
		{
			using MutableUserClass = typename TRemoveConst<UserClass>::Type;

			// Safely remove const to work around a compiler issue with instantiating template permutations for 
			// overloaded functions that take a function pointer typedef as a member of a templated class.  In
			// all cases where this code is actually invoked, the UserClass will already be a const pointer.
			MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(SharedUserObject.Get());

			checkSlow(MethodPtr != nullptr);

			(void)this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);

			return true;
		}

		return false;
	}

public:

	/**
	 * Creates a new shared pointer delegate binding for the given user object and method pointer.
	 *
	 * @param InUserObjectRef Shared reference to the user's object that contains the class method.
	 * @param InFunc Member function pointer to your class method.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, const TSharedPtr<UserClass, SPMode>& InUserObjectRef, FMethodPtr InFunc, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InUserObjectRef, InFunc, Vars...);
	}

	/**
	 * Creates a new shared pointer delegate binding for the given user object and method pointer.
	 *
	 * This overload requires that the supplied object derives from TSharedFromThis.
	 *
	 * @param InUserObject  The user's object that contains the class method.  Must derive from TSharedFromThis.
	 * @param InFunc  Member function pointer to your class method.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InUserObject, FMethodPtr InFunc, VarTypes... Vars)
	{
		// We expect the incoming InUserObject to derived from TSharedFromThis.
		TSharedRef<UserClass, SPMode> UserObjectRef = StaticCastSharedRef<UserClass>(InUserObject->AsShared());
		Create(Base, UserObjectRef, InFunc, Vars...);
	}

protected:

	// Weak reference to an instance of the user's class which contains a method we would like to call.
	TWeakPtr<UserClass, SPMode> UserObject;

	// C++ member function pointer.
	FMethodPtr MethodPtr;
};


/**
 * Implements a delegate binding for C++ member functions.
 */
template <bool bConst, class UserClass, typename FuncType, typename UserPolicy, typename... VarTypes>
class TBaseRawMethodDelegateInstance;

template <bool bConst, class UserClass, typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TBaseRawMethodDelegateInstance<bConst, UserClass, WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	static_assert(!UE4Delegates_Private::IsUObjectPtr((UserClass*)nullptr), "You cannot use raw method delegates with UObjects.");

	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseRawMethodDelegateInstance<bConst, UserClass, RetValType(ParamTypes...), UserPolicy, VarTypes...>;

public:
	using FMethodPtr = typename TMemFunPtrType<bConst, UserClass, RetValType(ParamTypes..., VarTypes...)>::Type;

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InUserObject An arbitrary object (templated) that hosts the member function.
	 * @param InMethodPtr C++ member function pointer for the method to bind.
	 */
	TBaseRawMethodDelegateInstance(UserClass* InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: Super     (Vars...)
		, UserObject(InUserObject)
		, MethodPtr (InMethodPtr)
	{
		// Non-expirable delegates must always have a non-null object pointer on creation (otherwise they could never execute.)
		check(InUserObject != nullptr && MethodPtr != nullptr);
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const final
	{
		return UserObject;
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
#if PLATFORM_64BITS
		return *((uint64*)&MethodPtr);
#else
		return *((uint32*)&MethodPtr);
#endif
	}

	// Deprecated
	bool HasSameObject(const void* InUserObject) const final
	{
		return UserObject == InUserObject;
	}

	bool IsSafeToExecute() const final
	{
		// We never know whether or not it is safe to deference a C++ pointer, but we have to
		// trust the user in this case.  Prefer using a shared-pointer based delegate type instead!
		return true;
	}

public:

	// IBaseDelegateInstance interface

	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		using MutableUserClass = typename TRemoveConst<UserClass>::Type;

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(UserObject);

		checkSlow(MethodPtr != nullptr);

		return this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);
	}


	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		using MutableUserClass = typename TRemoveConst<UserClass>::Type;

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(UserObject);

		checkSlow(MethodPtr != nullptr);

		(void)this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);

		return true;
	}

public:

	/**
	 * Creates a new raw method delegate binding for the given user object and function pointer.
	 *
	 * @param InUserObject User's object that contains the class method.
	 * @param InFunc Member function pointer to your class method.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InUserObject, FMethodPtr InFunc, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InUserObject, InFunc, Vars...);
	}

protected:

	// Pointer to the user's class which contains a method we would like to call.
	UserClass* UserObject;

	// C++ member function pointer.
	FMethodPtr MethodPtr;
};

/**
 * Implements a delegate binding for UObject methods.
 */
template <bool bConst, class UserClass, typename FuncType, typename UserPolicy, typename... VarTypes>
class TBaseUObjectMethodDelegateInstance;

template <bool bConst, class UserClass, typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TBaseUObjectMethodDelegateInstance<bConst, UserClass, WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseUObjectMethodDelegateInstance<bConst, UserClass, RetValType(ParamTypes...), UserPolicy, VarTypes...>;

	static_assert(UE4Delegates_Private::IsUObjectPtr((UserClass*)nullptr), "You cannot use UObject method delegates with raw pointers.");

public:
	using FMethodPtr = typename TMemFunPtrType<bConst, UserClass, RetValType(ParamTypes..., VarTypes...)>::Type;

	TBaseUObjectMethodDelegateInstance(UserClass* InUserObject, FMethodPtr InMethodPtr, VarTypes... Vars)
		: Super     (Vars...)
		, UserObject(InUserObject)
		, MethodPtr (InMethodPtr)
	{
		// NOTE: UObject delegates are allowed to have a null incoming object pointer.  UObject weak pointers can expire,
		//       an it is possible for a copy of a delegate instance to end up with a null pointer.
		checkSlow(MethodPtr != nullptr);
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return (UObject*)UserObject.Get();
	}

	const void* GetObjectForTimerManager() const final
	{
		return UserObject.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
#if PLATFORM_64BITS
		return *((uint64*)&MethodPtr);
#else
		return *((uint32*)&MethodPtr);
#endif
	}

	// Deprecated
	bool HasSameObject(const void* InUserObject) const final
	{
		return (UserObject.Get() == InUserObject);
	}

	bool IsCompactable() const final
	{
		return !UserObject.Get(true);
	}

	bool IsSafeToExecute() const final
	{
		return !!UserObject.Get();
	}

public:

	// IBaseDelegateInstance interface

	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		using MutableUserClass = typename TRemoveConst<UserClass>::Type;

		// Verify that the user object is still valid.  We only have a weak reference to it.
		checkSlow(UserObject.IsValid());

		// Safely remove const to work around a compiler issue with instantiating template permutations for 
		// overloaded functions that take a function pointer typedef as a member of a templated class.  In
		// all cases where this code is actually invoked, the UserClass will already be a const pointer.
		MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(UserObject.Get());

		checkSlow(MethodPtr != nullptr);

		return this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		if (UserClass* ActualUserObject = this->UserObject.Get())
		{
			using MutableUserClass = typename TRemoveConst<UserClass>::Type;

			// Safely remove const to work around a compiler issue with instantiating template permutations for 
			// overloaded functions that take a function pointer typedef as a member of a templated class.  In
			// all cases where this code is actually invoked, the UserClass will already be a const pointer.
			MutableUserClass* MutableUserObject = const_cast<MutableUserClass*>(ActualUserObject);

			checkSlow(MethodPtr != nullptr);

			(void)this->Payload.ApplyAfter(MethodPtr, MutableUserObject, Params...);

			return true;
		}
		return false;
	}

public:

	/**
	 * Creates a new UObject delegate binding for the given user object and method pointer.
	 *
	 * @param InUserObject User's object that contains the class method.
	 * @param InFunc Member function pointer to your class method.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InUserObject, FMethodPtr InFunc, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InUserObject, InFunc, Vars...);
	}

protected:

	// Pointer to the user's class which contains a method we would like to call.
	TWeakObjectPtr<UserClass> UserObject;

	// C++ member function pointer.
	FMethodPtr MethodPtr;
};


/**
 * Implements a delegate binding for regular C++ functions.
 */
template <typename FuncType, typename UserPolicy, typename... VarTypes>
class TBaseStaticDelegateInstance;

template <typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename... VarTypes>
class TBaseStaticDelegateInstance<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseStaticDelegateInstance<RetValType(ParamTypes...), UserPolicy, VarTypes...>;

public:
	using FFuncPtr = RetValType(*)(ParamTypes..., VarTypes...);

	TBaseStaticDelegateInstance(FFuncPtr InStaticFuncPtr, VarTypes... Vars)
		: Super        (Vars...)
		, StaticFuncPtr(InStaticFuncPtr)
	{
		check(StaticFuncPtr != nullptr);
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const final
	{
		return nullptr;
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
#if PLATFORM_64BITS
		return *((uint64*)&StaticFuncPtr);
#else
		return *((uint32*)&StaticFuncPtr);
#endif
	}

	// Deprecated
	bool HasSameObject(const void* UserObject) const final
	{
		// Raw Delegates aren't bound to an object so they can never match
		return false;
	}

	bool IsSafeToExecute() const final
	{
		// Static functions are always safe to execute!
		return true;
	}

public:

	// IBaseDelegateInstance interface

	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		// Call the static function
		checkSlow(StaticFuncPtr != nullptr);

		return this->Payload.ApplyAfter(StaticFuncPtr, Params...);
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		// Call the static function
		checkSlow(StaticFuncPtr != nullptr);

		(void)this->Payload.ApplyAfter(StaticFuncPtr, Params...);

		return true;
	}

public:

	/**
	 * Creates a new static function delegate binding for the given function pointer.
	 *
	 * @param InFunc Static function pointer.
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, FFuncPtr InFunc, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InFunc, Vars...);
	}

private:

	// C++ function pointer.
	FFuncPtr StaticFuncPtr;
};

/**
 * Implements a delegate binding for C++ functors, e.g. lambdas.
 */
template <typename FuncType, typename UserPolicy, typename FunctorType, typename... VarTypes>
class TBaseFunctorDelegateInstance;

template <typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename FunctorType, typename... VarTypes>
class TBaseFunctorDelegateInstance<WrappedRetValType(ParamTypes...), UserPolicy, FunctorType, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	static_assert(TAreTypesEqual<FunctorType, typename TRemoveReference<FunctorType>::Type>::Value, "FunctorType cannot be a reference");

	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TBaseFunctorDelegateInstance<RetValType(ParamTypes...), UserPolicy, FunctorType, VarTypes...>;

public:
	TBaseFunctorDelegateInstance(const FunctorType& InFunctor, VarTypes... Vars)
		: Super  (Vars...)
		, Functor(InFunctor)
	{
	}

	TBaseFunctorDelegateInstance(FunctorType&& InFunctor, VarTypes... Vars)
		: Super  (Vars...)
		, Functor(MoveTemp(InFunctor))
	{
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return nullptr;
	}

	const void* GetObjectForTimerManager() const final
	{
		return nullptr;
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
		return 0;
	}

	// Deprecated
	bool HasSameObject(const void* UserObject) const final
	{
		// Functor Delegates aren't bound to a user object so they can never match
		return false;
	}

	bool IsSafeToExecute() const final
	{
		// Functors are always considered safe to execute!
		return true;
	}

public:
	// IBaseDelegateInstance interface
	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		return this->Payload.ApplyAfter(Functor, Params...);
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		// Functors are always considered safe to execute!
		(void)this->Payload.ApplyAfter(Functor, Params...);

		return true;
	}

public:
	/**
	 * Creates a new static function delegate binding for the given function pointer.
	 *
	 * @param InFunctor C++ functor
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, const FunctorType& InFunctor, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InFunctor, Vars...);
	}
	FORCEINLINE static void Create(FDelegateBase& Base, FunctorType&& InFunctor, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(MoveTemp(InFunctor), Vars...);
	}

private:
	// C++ functor
	// We make this mutable to allow mutable lambdas to be bound and executed.  We don't really want to
	// model the Functor as being a direct subobject of the delegate (which would maintain transivity of
	// const - because the binding doesn't affect the substitutability of a copied delegate.
	mutable typename TRemoveConst<FunctorType>::Type Functor;
};

/**
 * Implements a weak object delegate binding for C++ functors, e.g. lambdas.
 */
template <typename UserClass, typename FuncType, typename UserPolicy, typename FunctorType, typename... VarTypes>
class TWeakBaseFunctorDelegateInstance;

template <typename UserClass, typename WrappedRetValType, typename... ParamTypes, typename UserPolicy, typename FunctorType, typename... VarTypes>
class TWeakBaseFunctorDelegateInstance<UserClass, WrappedRetValType(ParamTypes...), UserPolicy, FunctorType, VarTypes...> : public TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>
{
private:
	static_assert(TAreTypesEqual<FunctorType, typename TRemoveReference<FunctorType>::Type>::Value, "FunctorType cannot be a reference");

	using Super             = TCommonDelegateInstanceState<WrappedRetValType(ParamTypes...), UserPolicy, VarTypes...>;
	using RetValType        = typename Super::RetValType;
	using UnwrappedThisType = TWeakBaseFunctorDelegateInstance<UserClass, RetValType(ParamTypes...), UserPolicy, FunctorType, VarTypes...>;

public:
	TWeakBaseFunctorDelegateInstance(UserClass* InContextObject, const FunctorType& InFunctor, VarTypes... Vars)
		: Super        (Vars...)
		, ContextObject(InContextObject)
		, Functor      (InFunctor)
	{
	}

	TWeakBaseFunctorDelegateInstance(UserClass* InContextObject, FunctorType&& InFunctor, VarTypes... Vars)
		: Super        (Vars...)
		, ContextObject(InContextObject)
		, Functor      (MoveTemp(InFunctor))
	{
	}

	// IDelegateInstance interface

#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	FName TryGetBoundFunctionName() const final
	{
		return NAME_None;
	}

#endif

	UObject* GetUObject() const final
	{
		return ContextObject.Get();
	}

	const void* GetObjectForTimerManager() const final
	{
		return ContextObject.Get();
	}

	uint64 GetBoundProgramCounterForTimerManager() const final
	{
		return 0;
	}

	// Deprecated
	bool HasSameObject(const void* InContextObject) const final
	{
		return GetUObject() == InContextObject;
	}

	bool IsCompactable() const final
	{
		return !ContextObject.Get(true);
	}

	bool IsSafeToExecute() const final
	{
		return ContextObject.IsValid();
	}

public:
	// IBaseDelegateInstance interface
	void CreateCopy(FDelegateBase& Base) final
	{
		new (Base) UnwrappedThisType(*(UnwrappedThisType*)this);
	}

	RetValType Execute(ParamTypes... Params) const final
	{
		return this->Payload.ApplyAfter(Functor, Params...);
	}

	bool ExecuteIfSafe(ParamTypes... Params) const final
	{
		if (ContextObject.IsValid())
		{
			(void)this->Payload.ApplyAfter(Functor, Params...);
			return true;
		}

		return false;
	}

public:
	/**
	 * Creates a new static function delegate binding for the given function pointer.
	 *
	 * @param InFunctor C++ functor
	 * @return The new delegate.
	 */
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InContextObject, const FunctorType& InFunctor, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InContextObject, InFunctor, Vars...);
	}
	FORCEINLINE static void Create(FDelegateBase& Base, UserClass* InContextObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		new (Base) UnwrappedThisType(InContextObject, MoveTemp(InFunctor), Vars...);
	}

private:
	// Context object - the validity of this object controls the validity of the lambda
	TWeakObjectPtr<UserClass> ContextObject;

	// C++ functor
	// We make this mutable to allow mutable lambdas to be bound and executed.  We don't really want to
	// model the Functor as being a direct subobject of the delegate (which would maintain transivity of
	// const - because the binding doesn't affect the substitutability of a copied delegate.
	mutable typename TRemoveConst<FunctorType>::Type Functor;
};
