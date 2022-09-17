// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/NameTypes.h"
#include "Delegates/DelegateSettings.h"
#include "Delegates/IDelegateInstance.h"

struct FWeakObjectPtr;

#if !defined(_WIN32) || defined(_WIN64) || (defined(ALLOW_DELEGATE_INLINE_ALLOCATORS_ON_WIN32) && ALLOW_DELEGATE_INLINE_ALLOCATORS_ON_WIN32)
	typedef TAlignedBytes<16, 16> FAlignedInlineDelegateType;
	#if !defined(NUM_DELEGATE_INLINE_BYTES) || NUM_DELEGATE_INLINE_BYTES == 0
		typedef FHeapAllocator FDelegateAllocatorType;
	#elif NUM_DELEGATE_INLINE_BYTES < 0 || (NUM_DELEGATE_INLINE_BYTES % 16) != 0
		#error NUM_DELEGATE_INLINE_BYTES must be a multiple of 16
	#else
		typedef TInlineAllocator<(NUM_DELEGATE_INLINE_BYTES / 16)> FDelegateAllocatorType;
	#endif
#else
	// ... except on Win32, because we can't pass 16-byte aligned types by value, as some delegates are
	// so we'll just keep it heap-allocated, which are always sufficiently aligned.
	typedef TAlignedBytes<16, 8> FAlignedInlineDelegateType;
	typedef FHeapAllocator FDelegateAllocatorType;
#endif

struct FWeakObjectPtr;

template <typename UserPolicy>
class TMulticastDelegateBase;

ALIAS_TEMPLATE_TYPE_LAYOUT(template<typename ElementType>, FDelegateAllocatorType::ForElementType<ElementType>, void*);

/**
 * Base class for unicast delegates.
 */
class FDelegateBase //-V1062
{
	template <typename>
	friend class TMulticastDelegateBase;

	template <typename>
	friend class TDelegateBase;

protected:
	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDelegateInstance The delegate instance to assign.
	 */
	explicit FDelegateBase()
		: DelegateSize(0)
	{
	}

	~FDelegateBase()
	{
		Unbind();
	}

	/**
	 * Move constructor.
	 */
	FDelegateBase(FDelegateBase&& Other)
	{
		DelegateAllocator.MoveToEmpty(Other.DelegateAllocator);
		DelegateSize = Other.DelegateSize;
		Other.DelegateSize = 0;
	}

	/**
	 * Move assignment.
	 */
	FDelegateBase& operator=(FDelegateBase&& Other)
	{
		Unbind();
		DelegateAllocator.MoveToEmpty(Other.DelegateAllocator);
		DelegateSize = Other.DelegateSize;
		Other.DelegateSize = 0;
		return *this;
	}

	/**
	 * Unbinds this delegate
	 */
	FORCEINLINE void Unbind()
	{
		if (IDelegateInstance* Ptr = GetDelegateInstanceProtected())
		{
			Ptr->~IDelegateInstance();
			DelegateAllocator.ResizeAllocation(0, 0, sizeof(FAlignedInlineDelegateType));
			DelegateSize = 0;
		}
	}

	/**
	 * Gets the delegate instance.  Not intended for use by user code.
	 *
	 * @return The delegate instance.
	 * @see SetDelegateInstance
	 */
	FORCEINLINE IDelegateInstance* GetDelegateInstanceProtected() const
	{
		return DelegateSize ? (IDelegateInstance*)DelegateAllocator.GetAllocation() : nullptr;
	}

private:
	friend void* operator new(size_t Size, FDelegateBase& Base);

	void* Allocate(int32 Size)
	{
		if (IDelegateInstance* CurrentInstance = GetDelegateInstanceProtected())
		{
			CurrentInstance->~IDelegateInstance();
		}

		int32 NewDelegateSize = FMath::DivideAndRoundUp(Size, (int32)sizeof(FAlignedInlineDelegateType));
		if (DelegateSize != NewDelegateSize)
		{
			DelegateAllocator.ResizeAllocation(0, NewDelegateSize, sizeof(FAlignedInlineDelegateType));
			DelegateSize = NewDelegateSize;
		}

		return DelegateAllocator.GetAllocation();
	}

private:
	FDelegateAllocatorType::ForElementType<FAlignedInlineDelegateType> DelegateAllocator;
	int32 DelegateSize;
};

inline void* operator new(size_t Size, FDelegateBase& Base)
{
	return Base.Allocate((int32)Size);
}

struct FDefaultDelegateUserPolicy
{
	// To extend delegates, you should implement a policy struct like this and pass it as the second template
	// argument to TDelegate and TMulticastDelegate.  This policy struct containing three classes called:
	// 
	// FDelegateInstanceExtras:
	//   - Must publicly inherit IDelegateInstance.
	//   - Should contain any extra data and functions injected into a binding (the object which holds and
	//     is able to invoke the binding passed to FMyDelegate::CreateSP, FMyDelegate::CreateLambda etc.).
	//   - This binding is not available through the public API of the delegate, but is accessible to FDelegateExtras.
	//
	// FDelegateExtras:
	//   - Must publicly inherit FDelegateBase.
	//   - Should contain any extra data and functions injected into a delegate (the object which holds an
	//     FDelegateInstance-derived object, above).
	//   - Public data members and member functions are accessible directly through the TDelegate object.
	//   - Typically member functions in this class will forward calls to the inner FDelegateInstanceExtras,
	//     by downcasting the result of a call to GetDelegateInstanceProtected().
	//
	// FMulticastDelegateExtras:
	//   - Must publicly inherit TMulticastDelegateBase<FYourUserPolicyStruct>.
	//   - Should contain any extra data and functions injected into a multicast delegate (the object which
	//     holds an array of FDelegateExtras-derived objects which is the invocation list).
	//   - Public data members and member functions are accessible directly through the TMulticastDelegate object.

	using FDelegateInstanceExtras  = IDelegateInstance;
	using FDelegateExtras          = FDelegateBase;
	using FMulticastDelegateExtras = TMulticastDelegateBase<FDefaultDelegateUserPolicy>;
};

template <typename UserPolicy>
class TDelegateBase : public UserPolicy::FDelegateExtras
{
	template <typename>
	friend class TMulticastDelegateBase;

	using Super = typename UserPolicy::FDelegateExtras;

public:
#if USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

	/**
	 * Tries to return the name of a bound function.  Returns NAME_None if the delegate is unbound or
	 * a binding name is unavailable.
	 *
	 * Note: Only intended to be used to aid debugging of delegates.
	 *
	 * @return The name of the bound function, NAME_None if no name was available.
	 */
	FName TryGetBoundFunctionName() const
	{
		if (IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected())
		{
			return Ptr->TryGetBoundFunctionName();
		}

		return NAME_None;
	}

#endif

	/**
	 * If this is a UFunction or UObject delegate, return the UObject.
	 *
	 * @return The object associated with this delegate if there is one.
	 */
	FORCEINLINE class UObject* GetUObject( ) const
	{
		if (IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected())
		{
			return Ptr->GetUObject();
		}

		return nullptr;
	}

	/**
	 * Checks to see if the user object bound to this delegate is still valid.
	 *
	 * @return True if the user object is still valid and it's safe to execute the function call.
	 */
	FORCEINLINE bool IsBound( ) const
	{
		IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected();

		return Ptr && Ptr->IsSafeToExecute();
	}

	/** 
	 * Returns a pointer to an object bound to this delegate, intended for quick lookup in the timer manager,
	 *
	 * @return A pointer to an object referenced by the delegate.
	 */
	FORCEINLINE const void* GetObjectForTimerManager() const
	{
		IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected();

		const void* Result = Ptr ? Ptr->GetObjectForTimerManager() : nullptr;
		return Result;
	}

	/**
	 * Returns the address of the method pointer which can be used to learn the address of the function that will be executed.
	 * Returns nullptr if this delegate type does not directly invoke a function pointer.
	 *
	 * Note: Only intended to be used to aid debugging of delegates.
	 *
	 * @return The address of the function pointer that would be executed by this delegate
	 */
	uint64 GetBoundProgramCounterForTimerManager() const
	{
		if (IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected())
		{
			return Ptr->GetBoundProgramCounterForTimerManager();
		}

		return 0;
	}

	/** 
	 * Checks to see if this delegate is bound to the given user object.
	 *
	 * @return True if this delegate is bound to InUserObject, false otherwise.
	 */
	FORCEINLINE bool IsBoundToObject( void const* InUserObject ) const
	{
		if (!InUserObject)
		{
			return false;
		}

		IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected();

		return Ptr && Ptr->HasSameObject(InUserObject);
	}

	/**
	 * Gets a handle to the delegate.
	 *
	 * @return The delegate instance.
	 */
	FORCEINLINE FDelegateHandle GetHandle() const
	{
		FDelegateHandle Result;
		if (IDelegateInstance* Ptr = Super::GetDelegateInstanceProtected())
		{
			Result = Ptr->GetHandle();
		}

		return Result;
	}
};
