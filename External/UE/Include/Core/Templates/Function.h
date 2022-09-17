// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "Templates/AndOrNot.h"
#include "Templates/ChooseClass.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/RemoveReference.h"
#include "Templates/Decay.h"
#include "Templates/Invoke.h"
#include "Templates/IsConstructible.h"
#include "Templates/IsInvocable.h"
#include "Templates/IsMemberPointer.h"
#include "Templates/IsPointer.h"
#include "Templates/UnrealTemplate.h"
#include "Math/UnrealMathUtility.h"
#include <new>


// Disable visualization hack for shipping or test builds.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	#define ENABLE_TFUNCTIONREF_VISUALIZATION 1
#else
	#define ENABLE_TFUNCTIONREF_VISUALIZATION 0
#endif

#if defined(_WIN32) && !defined(_WIN64) && (!defined(ALLOW_TFUNCTION_INLINE_ALLOCATORS_ON_WIN32) || !ALLOW_TFUNCTION_INLINE_ALLOCATORS_ON_WIN32)
	// Don't use inline storage on Win32, because that will affect the alignment of TFunction, and we can't pass extra-aligned types by value on Win32.
	#define TFUNCTION_USES_INLINE_STORAGE 0
#elif !defined(NUM_TFUNCTION_INLINE_BYTES) || NUM_TFUNCTION_INLINE_BYTES == 0
	#define TFUNCTION_USES_INLINE_STORAGE 0
#else
	#define TFUNCTION_USES_INLINE_STORAGE 1
	#define TFUNCTION_INLINE_SIZE         NUM_TFUNCTION_INLINE_BYTES
	#define TFUNCTION_INLINE_ALIGNMENT    16
#endif

template <typename FuncType> class TFunction;
template <typename FuncType> class TFunctionRef;

/**
 * TFunction<FuncType>
 *
 * See the class definition for intended usage.
 */
template <typename FuncType>
class TFunction;

/**
 * TUniqueFunction<FuncType>
 *
 * See the class definition for intended usage.
 */
template <typename FuncType>
class TUniqueFunction;

/**
 * TFunctionRef<FuncType>
 *
 * See the class definition for intended usage.
 */
template <typename FuncType>
class TFunctionRef;

/**
 * Traits class which checks if T is a TFunction<> type.
 */
template <typename T> struct TIsTFunction               { enum { Value = false }; };
template <typename T> struct TIsTFunction<TFunction<T>> { enum { Value = true  }; };

template <typename T> struct TIsTFunction<const          T> { enum { Value = TIsTFunction<T>::Value }; };
template <typename T> struct TIsTFunction<      volatile T> { enum { Value = TIsTFunction<T>::Value }; };
template <typename T> struct TIsTFunction<const volatile T> { enum { Value = TIsTFunction<T>::Value }; };

/**
 * Traits class which checks if T is a TFunction<> type.
 */
template <typename T> struct TIsTUniqueFunction                     { enum { Value = false }; };
template <typename T> struct TIsTUniqueFunction<TUniqueFunction<T>> { enum { Value = true  }; };

template <typename T> struct TIsTUniqueFunction<const          T> { enum { Value = TIsTUniqueFunction<T>::Value }; };
template <typename T> struct TIsTUniqueFunction<      volatile T> { enum { Value = TIsTUniqueFunction<T>::Value }; };
template <typename T> struct TIsTUniqueFunction<const volatile T> { enum { Value = TIsTUniqueFunction<T>::Value }; };

/**
 * Traits class which checks if T is a TFunctionRef<> type.
 */
template <typename T> struct TIsTFunctionRef                  { enum { Value = false }; };
template <typename T> struct TIsTFunctionRef<TFunctionRef<T>> { enum { Value = true  }; };

template <typename T> struct TIsTFunctionRef<const          T> { enum { Value = TIsTFunctionRef<T>::Value }; };
template <typename T> struct TIsTFunctionRef<      volatile T> { enum { Value = TIsTFunctionRef<T>::Value }; };
template <typename T> struct TIsTFunctionRef<const volatile T> { enum { Value = TIsTFunctionRef<T>::Value }; };

/**
 * Private implementation details of TFunction and TFunctionRef.
 */
namespace UE::Core::Private::Function
{
	template <typename T, bool bOnHeap>
	struct TFunction_OwnedObject;

	template <bool bUnique>
	struct TFunctionStorage;

	/**
	 * Common interface to a callable object owned by TFunction.
	 */
	struct IFunction_OwnedObject
	{
		/**
		 * Creates a copy of itself into the storage and returns a pointer to the new object within it.
		 */
		virtual void* CloneToEmptyStorage(void* Storage) const = 0;

		/**
		 * Returns the address of the object.
		 */
		virtual void* GetAddress() = 0;

		/**
		 * Destructor.
		 */
		virtual void Destroy() = 0;

		/**
		 * Destructor.
		 */
		virtual ~IFunction_OwnedObject() = default;
	};

	/**
	 * Common interface to a callable object owned by TFunction.
	 */
	template <typename T>
	struct IFunction_OwnedObject_OnHeap : public IFunction_OwnedObject
	{
		/**
		 * Destructor.
		 */
		virtual void Destroy() override
		{
			void* This = this;
			this->~IFunction_OwnedObject_OnHeap();
			FMemory::Free(This);
		}

		~IFunction_OwnedObject_OnHeap() override
		{
			// It is not necessary to define this destructor but MSVC will
			// erroneously issue warning C5046 without it.
		}
	};

	/**
	 * Common interface to a callable object owned by TFunction.
	 */
	template <typename T>
	struct IFunction_OwnedObject_Inline : public IFunction_OwnedObject
	{
		/**
		 * Destructor.
		 */
		virtual void Destroy() override
		{
			this->~IFunction_OwnedObject_Inline();
		}

		~IFunction_OwnedObject_Inline() override
		{
			// It is not necessary to define this destructor but MSVC will
			// erroneously issue warning C5046 without it.
		}
	};

	template <typename T, bool bOnHeap>
	struct TFunction_OwnedObject : public
#if TFUNCTION_USES_INLINE_STORAGE
		TChooseClass<bOnHeap, IFunction_OwnedObject_OnHeap<T>, IFunction_OwnedObject_Inline<T>>::Result
#else
		IFunction_OwnedObject_OnHeap<T>
#endif
	{
		template <typename... ArgTypes>
		explicit TFunction_OwnedObject(ArgTypes&&... Args)
			: Obj(Forward<ArgTypes>(Args)...)
		{
		}

		virtual void* GetAddress() override
		{
			return &Obj;
		}

		T Obj;
	};

	/**
	 * Implementation of IFunction_OwnedObject for a given copyable T.
	 */
	template <typename T, bool bOnHeap>
	struct TFunction_CopyableOwnedObject final : public TFunction_OwnedObject<T, bOnHeap>
	{
		/**
		 * Constructor which creates its T by copying.
		 */
		explicit TFunction_CopyableOwnedObject(const T& InObj)
			: TFunction_OwnedObject<T, bOnHeap>(InObj)
		{
		}

		/**
		 * Constructor which creates its T by moving.
		 */
		explicit TFunction_CopyableOwnedObject(T&& InObj)
			: TFunction_OwnedObject<T, bOnHeap>(MoveTemp(InObj))
		{
		}

		void* CloneToEmptyStorage(void* UntypedStorage) const override;
	};

	/**
	 * Implementation of IFunction_OwnedObject for a given non-copyable T.
	 */
	template <typename T, bool bOnHeap>
	struct TFunction_UniqueOwnedObject final : public TFunction_OwnedObject<T, bOnHeap>
	{
		/**
		 * Constructor which creates its T by moving.
		 */
		explicit TFunction_UniqueOwnedObject(T&& InObj)
			: TFunction_OwnedObject<T, bOnHeap>(MoveTemp(InObj))
		{
		}

		void* CloneToEmptyStorage(void* Storage) const override
		{
			// Should never get here - copy functions are deleted for TUniqueFunction
			check(false);
			return nullptr;
		}
	};

	template <typename T>
	struct TIsNullableBinding :
		TOr<
			TIsPointer<T>,
			TIsMemberPointer<T>,
			TIsTFunction<T>
		>
	{
	};

#if PLATFORM_COMPILER_HAS_IF_CONSTEXPR

	template <typename T>
	FORCEINLINE bool IsBound(const T& Func)
	{
		if constexpr (TIsNullableBinding<T>::Value)
		{
			// Function pointers, data member pointers, member function pointers and TFunctions
			// can all be null/unbound, so test them using their boolean state.
			return !!Func;
		}
		else
		{
			// We can't tell if any other generic callable can be invoked, so just assume they can be.
			return true;
		}
	}

#else

	template <typename T>
	FORCEINLINE typename TEnableIf<TIsNullableBinding<T>::Value, bool>::Type IsBound(const T& Func)
	{
		// Function pointers, data member pointers, member function pointers and TFunctions
		// can all be null/unbound, so test them using their boolean state.
		return !!Func;
	}

	template <typename T>
	FORCEINLINE typename TEnableIf<!TIsNullableBinding<T>::Value, bool>::Type IsBound(const T& Func)
	{
		// We can't tell if any other generic callable can be invoked, so just assume they can be.
		return true;
	}

#endif

	template <typename FunctorType, bool bUnique, bool bOnHeap>
	struct TStorageOwnerType;

	template <typename FunctorType, bool bOnHeap>
	struct TStorageOwnerType<FunctorType, true, bOnHeap>
	{
		using Type = TFunction_UniqueOwnedObject<typename TDecay<FunctorType>::Type, bOnHeap>;
	};

	template <typename FunctorType, bool bOnHeap>
	struct TStorageOwnerType<FunctorType, false, bOnHeap>
	{
		using Type = TFunction_CopyableOwnedObject<typename TDecay<FunctorType>::Type, bOnHeap>;
	};

	template <typename FunctorType, bool bUnique, bool bOnHeap>
	using TStorageOwnerTypeT = typename TStorageOwnerType<FunctorType, bUnique, bOnHeap>::Type;

	struct FFunctionStorage
	{
		FFunctionStorage()
			: HeapAllocation(nullptr)
		{
		}

		FFunctionStorage(FFunctionStorage&& Other)
			: HeapAllocation(Other.HeapAllocation)
		{
			Other.HeapAllocation = nullptr;
			#if TFUNCTION_USES_INLINE_STORAGE
				FMemory::Memcpy(&InlineAllocation, &Other.InlineAllocation, sizeof(InlineAllocation));
			#endif
		}

		FFunctionStorage(const FFunctionStorage& Other) = delete;
		FFunctionStorage& operator=(FFunctionStorage&& Other) = delete;
		FFunctionStorage& operator=(const FFunctionStorage& Other) = delete;

		void* BindCopy(const FFunctionStorage& Other)
		{
			void* NewObj = Other.GetBoundObject()->CloneToEmptyStorage(this);
			return NewObj;
		}

		IFunction_OwnedObject* GetBoundObject() const
		{
			IFunction_OwnedObject* Result = (IFunction_OwnedObject*)HeapAllocation;
			#if TFUNCTION_USES_INLINE_STORAGE
				if (!Result)
				{
					Result = (IFunction_OwnedObject*)&InlineAllocation;
				}
			#endif

			return Result;
		}

		/**
		 * Returns a pointer to the callable object - needed by TFunctionRefBase.
		 */
		void* GetPtr() const
		{
		#if TFUNCTION_USES_INLINE_STORAGE
			IFunction_OwnedObject* Owned = (IFunction_OwnedObject*)HeapAllocation;
			if (!Owned)
			{
				Owned = (IFunction_OwnedObject*)&InlineAllocation;
			}

			return Owned->GetAddress();
		#else
			return ((IFunction_OwnedObject*)HeapAllocation)->GetAddress();
		#endif
		}

		/**
		 * Destroy any owned bindings - called by TFunctionRefBase only if Bind() or BindCopy() was called.
		 */
		void Unbind()
		{
			IFunction_OwnedObject* Owned = GetBoundObject();
			Owned->Destroy();
		}

		void* HeapAllocation;
		#if TFUNCTION_USES_INLINE_STORAGE
			// Inline storage for an owned object
			TAlignedBytes<TFUNCTION_INLINE_SIZE, TFUNCTION_INLINE_ALIGNMENT> InlineAllocation;
		#endif
	};

	template <bool bUnique>
	struct TFunctionStorage : FFunctionStorage
	{
		TFunctionStorage() = default;

		TFunctionStorage(FFunctionStorage&& Other)
			: FFunctionStorage(MoveTemp(Other))
		{
		}

#if PLATFORM_COMPILER_HAS_IF_CONSTEXPR

		template <typename FunctorType>
		typename TDecay<FunctorType>::Type* Bind(FunctorType&& InFunc)
		{
			if (!IsBound(InFunc))
			{
				return nullptr;
			}

#if TFUNCTION_USES_INLINE_STORAGE
			constexpr bool bUseInline = sizeof(TStorageOwnerTypeT<FunctorType, bUnique, false>) <= TFUNCTION_INLINE_SIZE;
#else
			constexpr bool bUseInline = false;
#endif

			using OwnedType = TStorageOwnerTypeT<FunctorType, bUnique, !bUseInline>;

			void* NewAlloc;
#if TFUNCTION_USES_INLINE_STORAGE
			if constexpr (bUseInline)
			{
				NewAlloc = &InlineAllocation;
			}
			else
#endif
			{
				NewAlloc = FMemory::Malloc(sizeof(OwnedType), alignof(OwnedType));
				HeapAllocation = NewAlloc;
			}

			CA_ASSUME(NewAlloc);
			auto* NewOwned = new (NewAlloc) OwnedType(Forward<FunctorType>(InFunc));
			return &NewOwned->Obj;
		}

#else

		#if TFUNCTION_USES_INLINE_STORAGE
		template <typename FunctorType>
		typename TEnableIf<
			(sizeof(TStorageOwnerTypeT<FunctorType, bUnique, false>) <= TFUNCTION_INLINE_SIZE),
			typename TDecay<FunctorType>::Type*
		>::Type Bind(FunctorType&& InFunc)
		{
			if (!IsBound(InFunc))
			{
				return nullptr;
			}

			using OwnedType = TStorageOwnerTypeT<FunctorType, bUnique, false>;

			void* NewAlloc = &InlineAllocation;
			auto* NewOwned = new (NewAlloc) OwnedType(Forward<FunctorType>(InFunc));

			return &NewOwned->Obj;
		}
		#endif

		template <typename FunctorType>
		#if TFUNCTION_USES_INLINE_STORAGE
		typename TEnableIf<
			(sizeof(TStorageOwnerTypeT<FunctorType, bUnique, true>) > TFUNCTION_INLINE_SIZE),
			typename TDecay<FunctorType>::Type*
		>::Type Bind(FunctorType&& InFunc)
		#else
		typename TDecay<FunctorType>::Type* Bind(FunctorType&& InFunc)
		#endif
		{
			if (!IsBound(InFunc))
			{
				return nullptr;
			}

			using OwnedType = TStorageOwnerTypeT<FunctorType, bUnique, true>;

			void* NewAlloc = FMemory::Malloc(sizeof(OwnedType), alignof(OwnedType));
			CA_ASSUME(NewAlloc);
			auto* NewOwned = new (NewAlloc) OwnedType(Forward<FunctorType>(InFunc));

			HeapAllocation = NewAlloc;

			return &NewOwned->Obj;
		}

#endif
	};

	template <typename T, bool bOnHeap>
	void* TFunction_CopyableOwnedObject<T, bOnHeap>::CloneToEmptyStorage(void* UntypedStorage) const
	{
		TFunctionStorage<false>& Storage = *(TFunctionStorage<false>*)UntypedStorage;

		void* NewAlloc;
		#if TFUNCTION_USES_INLINE_STORAGE
		if /* constexpr */ (!bOnHeap)
		{
			NewAlloc = &Storage.InlineAllocation;
		}
		else
		#endif
		{
			NewAlloc = FMemory::Malloc(sizeof(TFunction_CopyableOwnedObject), alignof(TFunction_CopyableOwnedObject));
			Storage.HeapAllocation = NewAlloc;
			CA_ASSUME(NewAlloc);
		}

		auto* NewOwned = new (NewAlloc) TFunction_CopyableOwnedObject(this->Obj);

		return &NewOwned->Obj;
	}

	#if ENABLE_TFUNCTIONREF_VISUALIZATION
		/**
		 * Helper classes to help debugger visualization.
		 */
		struct IDebugHelper
		{
			virtual ~IDebugHelper() = 0;
		};

		inline IDebugHelper::~IDebugHelper()
		{
		}

		template <typename T>
		struct TDebugHelper : IDebugHelper
		{
			T* Ptr = nullptr;
		};
	#endif

	/**
	 * A class which is used to instantiate the code needed to call a bound function.
	 */
	template <typename Functor, typename FuncType>
	struct TFunctionRefCaller;

	template <typename Functor, typename Ret, typename... ParamTypes>
	struct TFunctionRefCaller<Functor, Ret (ParamTypes...)>
	{
		static Ret Call(void* Obj, ParamTypes&... Params)
		{
			return Invoke(*(Functor*)Obj, Forward<ParamTypes>(Params)...);
		}
	};

	template <typename Functor, typename... ParamTypes>
	struct TFunctionRefCaller<Functor, void (ParamTypes...)>
	{
		static void Call(void* Obj, ParamTypes&... Params)
		{
			Invoke(*(Functor*)Obj, Forward<ParamTypes>(Params)...);
		}
	};

	/**
	 * A class which defines an operator() which will own storage of a callable and invoke the TFunctionRefCaller::Call function on it.
	 */
	template <typename StorageType, typename FuncType>
	struct TFunctionRefBase;

	template <typename StorageType, typename Ret, typename... ParamTypes>
	struct TFunctionRefBase<StorageType, Ret (ParamTypes...)>
	{
		template <typename OtherStorageType, typename OtherFuncType>
		friend struct TFunctionRefBase;

		TFunctionRefBase()
			: Callable(nullptr)
		{
		}

		TFunctionRefBase(TFunctionRefBase&& Other)
			: Callable(Other.Callable)
			, Storage (MoveTemp(Other.Storage))
		{
			if (Callable)
			{
				#if ENABLE_TFUNCTIONREF_VISUALIZATION
					// Use Memcpy to copy the other DebugPtrStorage, including vptr (because we don't know the bound type
					// here), and then reseat the underlying pointer.  Possibly even more evil than the Set code.
					FMemory::Memcpy(&DebugPtrStorage, &Other.DebugPtrStorage, sizeof(DebugPtrStorage)); //-V598
					DebugPtrStorage.Ptr = Storage.GetPtr();
				#endif

				Other.Callable = nullptr;
			}
		}

		template <typename OtherStorage>
		TFunctionRefBase(TFunctionRefBase<OtherStorage, Ret (ParamTypes...)>&& Other)
			: Callable(Other.Callable)
			, Storage (MoveTemp(Other.Storage))
		{
			if (Callable)
			{
				#if ENABLE_TFUNCTIONREF_VISUALIZATION
					// Use Memcpy to copy the other DebugPtrStorage, including vptr (because we don't know the bound type
					// here), and then reseat the underlying pointer.  Possibly even more evil than the Set code.
					FMemory::Memcpy(&DebugPtrStorage, &Other.DebugPtrStorage, sizeof(DebugPtrStorage)); //-V598
					DebugPtrStorage.Ptr = Storage.GetPtr();
				#endif

				Other.Callable = nullptr;
			}
		}

		template <typename OtherStorage>
		TFunctionRefBase(const TFunctionRefBase<OtherStorage, Ret (ParamTypes...)>& Other)
			: Callable(Other.Callable)
		{
			if (Callable)
			{
				void* NewPtr = Storage.BindCopy(Other.Storage);

				#if ENABLE_TFUNCTIONREF_VISUALIZATION
					// Use Memcpy to copy the other DebugPtrStorage, including vptr (because we don't know the bound type
					// here), and then reseat the underlying pointer.  Possibly even more evil than the Set code.
					FMemory::Memcpy(&DebugPtrStorage, &Other.DebugPtrStorage, sizeof(DebugPtrStorage)); //-V598
					DebugPtrStorage.Ptr = NewPtr;
				#endif
			}
		}

		TFunctionRefBase(const TFunctionRefBase& Other)
			: Callable(Other.Callable)
		{
			if (Callable)
			{
				void* NewPtr = Storage.BindCopy(Other.Storage);

				#if ENABLE_TFUNCTIONREF_VISUALIZATION
					// Use Memcpy to copy the other DebugPtrStorage, including vptr (because we don't know the bound type
					// here), and then reseat the underlying pointer.  Possibly even more evil than the Set code.
					FMemory::Memcpy(&DebugPtrStorage, &Other.DebugPtrStorage, sizeof(DebugPtrStorage)); //-V598
					DebugPtrStorage.Ptr = NewPtr;
				#endif
			}
		}

		template <
			typename FunctorType,
			typename = typename TEnableIf<
				TNot<
					TIsSame<TFunctionRefBase, typename TDecay<FunctorType>::Type>
				>::Value
			>::Type
		>
		TFunctionRefBase(FunctorType&& InFunc)
		{
			if (auto* Binding = Storage.Bind(Forward<FunctorType>(InFunc)))
			{
				using DecayedFunctorType = typename TRemovePointer<decltype(Binding)>::Type;

				Callable = &TFunctionRefCaller<DecayedFunctorType, Ret (ParamTypes...)>::Call;

				#if ENABLE_TFUNCTIONREF_VISUALIZATION
					// We placement new over the top of the same object each time.  This is illegal,
					// but it ensures that the vptr is set correctly for the bound type, and so is
					// visualizable.  We never depend on the state of this object at runtime, so it's
					// ok.
					new ((void*)&DebugPtrStorage) TDebugHelper<DecayedFunctorType>;
					DebugPtrStorage.Ptr = (void*)Binding;
				#endif
			}
		}

		TFunctionRefBase& operator=(TFunctionRefBase&&) = delete;
		TFunctionRefBase& operator=(const TFunctionRefBase&) = delete;

		// Move all of the assert code out of line
		void CheckCallable() const
		{
			checkf(Callable, TEXT("Attempting to call an unbound TFunction!"));
		}

		Ret operator()(ParamTypes... Params) const
		{
			CheckCallable();
			return Callable(Storage.GetPtr(), Params...);
		}

		~TFunctionRefBase()
		{
			if (Callable)
			{
				Storage.Unbind();
			}
		}

	protected:
		bool IsSet() const
		{
			return !!Callable;
		}

	private:
		// A pointer to a function which invokes the call operator on the callable object
		Ret (*Callable)(void*, ParamTypes&...) = nullptr;

		StorageType Storage;

		#if ENABLE_TFUNCTIONREF_VISUALIZATION
			// To help debug visualizers
			TDebugHelper<void> DebugPtrStorage;
		#endif
	};

	template <typename FunctorType, typename Ret, typename... ParamTypes>
	struct TFunctorReturnTypeIsCompatible
		: TIsConstructible<Ret, decltype(DeclVal<FunctorType>()(DeclVal<ParamTypes>()...))>
	{
	};

	template <typename MemberRet, typename Class, typename Ret, typename... ParamTypes>
	struct TFunctorReturnTypeIsCompatible<MemberRet Class::*, Ret, ParamTypes...>
		: TIsConstructible<Ret, MemberRet>
	{
	};

	template <typename MemberRet, typename Class, typename Ret, typename... ParamTypes>
	struct TFunctorReturnTypeIsCompatible<MemberRet Class::* const, Ret, ParamTypes...>
		: TIsConstructible<Ret, MemberRet>
	{
	};

	template <typename MemberRet, typename Class, typename... MemberParamTypes, typename Ret, typename... ParamTypes>
	struct TFunctorReturnTypeIsCompatible<MemberRet (Class::*)(MemberParamTypes...), Ret, ParamTypes...>
		: TIsConstructible<Ret, MemberRet>
	{
	};

	template <typename MemberRet, typename Class, typename... MemberParamTypes, typename Ret, typename... ParamTypes>
	struct TFunctorReturnTypeIsCompatible<MemberRet (Class::*)(MemberParamTypes...) const, Ret, ParamTypes...>
		: TIsConstructible<Ret, MemberRet>
	{
	};

	template <typename FuncType, typename FunctorType>
	struct TFuncCanBindToFunctor;

	template <typename FunctorType, typename Ret, typename... ParamTypes>
	struct TFuncCanBindToFunctor<Ret(ParamTypes...), FunctorType> :
		TAnd<
			TIsInvocable<FunctorType, ParamTypes...>,
			TFunctorReturnTypeIsCompatible<FunctorType, Ret, ParamTypes...>
		>
	{
	};

	template <typename FunctorType, typename... ParamTypes>
	struct TFuncCanBindToFunctor<void(ParamTypes...), FunctorType> :
		TIsInvocable<FunctorType, ParamTypes...>
	{
	};

	struct FFunctionRefStoragePolicy
	{
		template <typename FunctorType>
		typename TRemoveReference<FunctorType>::Type* Bind(FunctorType&& InFunc)
		{
			checkf(IsBound(InFunc), TEXT("Cannot bind a null/unbound callable to a TFunctionRef"));

			Ptr = (void*)&InFunc;
			return &InFunc;
		}

		void* BindCopy(const FFunctionRefStoragePolicy& Other)
		{
			void* OtherPtr = Other.Ptr;
			Ptr = OtherPtr;
			return OtherPtr;
		}

		/**
		 * Returns a pointer to the callable object - needed by TFunctionRefBase.
		 */
		void* GetPtr() const
		{
			return Ptr;
		}

		/**
		 * Destroy any owned bindings - called by TFunctionRefBase only if Bind() or BindCopy() was called.
		 */
		void Unbind() const
		{
			// FunctionRefs don't own their binding - do nothing
		}

	private:
		// A pointer to the callable object
		void* Ptr = nullptr;
	};
}

/**
 * TFunctionRef<FuncType>
 *
 * A class which represents a reference to something callable.  The important part here is *reference* - if
 * you bind it to a lambda and the lambda goes out of scope, you will be left with an invalid reference.
 *
 * FuncType represents a function type and so TFunctionRef should be defined as follows:
 *
 * // A function taking a string and float and returning int32.  Parameter names are optional.
 * TFunctionRef<int32 (const FString& Name, float Scale)>
 *
 * If you also want to take ownership of the callable thing, e.g. you want to return a lambda from a
 * function, you should use TFunction.  TFunctionRef does not concern itself with ownership because it's
 * intended to be FAST.
 *
 * TFunctionRef is most useful when you want to parameterize a function with some caller-defined code
 * without making it a template.
 *
 * Example:
 *
 * // Something.h
 * void DoSomethingWithConvertingStringsToInts(TFunctionRef<int32 (const FString& Str)> Convert);
 *
 * // Something.cpp
 * void DoSomethingWithConvertingStringsToInts(TFunctionRef<int32 (const FString& Str)> Convert)
 * {
 *     for (const FString& Str : SomeBunchOfStrings)
 *     {
 *         int32 Int = Convert(Str);
 *         DoSomething(Int);
 *     }
 * }
 *
 * // SomewhereElse.cpp
 * #include "Something.h"
 *
 * void Func()
 * {
 *     // First do something using string length
 *     DoSomethingWithConvertingStringsToInts([](const FString& Str) {
 *         return Str.Len();
 *     });
 *
 *     // Then do something using string conversion
 *     DoSomethingWithConvertingStringsToInts([](const FString& Str) {
 *         int32 Result;
 *         TTypeFromString<int32>::FromString(Result, *Str);
 *         return Result;
 *     });
 * }
 */
template <typename FuncType>
class TFunctionRef : public UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::FFunctionRefStoragePolicy, FuncType>
{
	using Super = UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::FFunctionRefStoragePolicy, FuncType>;

public:
	/**
	 * Constructor which binds a TFunctionRef to a callable object.
	 */
	template <
		typename FunctorType,
		typename = typename TEnableIf<
			TAnd<
				TNot<TIsTFunctionRef<typename TDecay<FunctorType>::Type>>,
				UE::Core::Private::Function::TFuncCanBindToFunctor<FuncType, typename TDecay<FunctorType>::Type>
			>::Value
		>::Type
	>
	TFunctionRef(FunctorType&& InFunc)
		: Super(Forward<FunctorType>(InFunc))
	{
		// This constructor is disabled for TFunctionRef types so it isn't incorrectly selected as copy/move constructors.
	}

	TFunctionRef(const TFunctionRef&) = default;

	// We delete the assignment operators because we don't want it to be confused with being related to
	// regular C++ reference assignment - i.e. calling the assignment operator of whatever the reference
	// is bound to - because that's not what TFunctionRef does, nor is it even capable of doing that.
	TFunctionRef& operator=(const TFunctionRef&) const = delete;
	~TFunctionRef() = default;
};

/**
 * TFunction<FuncType>
 *
 * A class which represents a copy of something callable.  FuncType represents a function type and so
 * TFunction should be defined as follows:
 *
 * // A function taking a string and float and returning int32.  Parameter names are optional.
 * TFunction<int32 (const FString& Name, float Scale)>
 *
 * Unlike TFunctionRef, this object is intended to be used like a UE version of std::function.  That is,
 * it takes a copy of whatever is bound to it, meaning you can return it from functions and store them in
 * objects without caring about the lifetime of the original object being bound.
 *
 * Example:
 *
 * // Something.h
 * TFunction<FString (int32)> GetTransform();
 *
 * // Something.cpp
 * TFunction<FString (int32)> GetTransform(const FString& Prefix)
 * {
 *     // Squares number and returns it as a string with the specified prefix
 *     return [=](int32 Num) {
 *         return Prefix + TEXT(": ") + TTypeToString<int32>::ToString(Num * Num);
 *     };
 * }
 *
 * // SomewhereElse.cpp
 * #include "Something.h"
 *
 * void Func()
 * {
 *     TFunction<FString (int32)> Transform = GetTransform(TEXT("Hello"));
 *
 *     FString Result = Transform(5); // "Hello: 25"
 * }
 */
template <typename FuncType>
class TFunction final : public UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>
{
	using Super = UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>;

public:
	/**
	 * Default constructor.
	 */
	TFunction(TYPE_OF_NULLPTR = nullptr)
	{
	}

	/**
	 * Constructor which binds a TFunction to any function object.
	 */
	template <
		typename FunctorType,
		typename = typename TEnableIf<
			TAnd<
				TNot<TIsTFunction<typename TDecay<FunctorType>::Type>>,
				UE::Core::Private::Function::TFuncCanBindToFunctor<FuncType, FunctorType>
			>::Value
		>::Type
	>
	TFunction(FunctorType&& InFunc)
		: Super(Forward<FunctorType>(InFunc))
	{
		// This constructor is disabled for TFunction types so it isn't incorrectly selected as copy/move constructors.

		// This is probably a mistake if you expect TFunction to take a copy of what
		// TFunctionRef is bound to, because that's not possible.
		//
		// If you really intended to bind a TFunction to a TFunctionRef, you can just
		// wrap it in a lambda (and thus it's clear you're just binding to a call to another
		// reference):
		//
		// TFunction<int32(float)> MyFunction = [MyFunctionRef](float F) { return MyFunctionRef(F); };
		static_assert(!TIsTFunctionRef<typename TDecay<FunctorType>::Type>::Value, "Cannot construct a TFunction from a TFunctionRef");
	}

	TFunction(TFunction&&) = default;
	TFunction(const TFunction& Other) = default;
	~TFunction() = default;

	/**
	 * Move assignment operator.
	 */
	TFunction& operator=(TFunction&& Other)
	{
		Swap(*this, Other);
		return *this;
	}

	/**
	 * Copy assignment operator.
	 */
	TFunction& operator=(const TFunction& Other)
	{
		TFunction Temp = Other;
		Swap(*this, Temp);
		return *this;
	}

	/**
	 * Removes any bound callable from the TFunction, restoring it to the default 'empty' state.
	 */
	void Reset()
	{
		*this = nullptr;
	}

	/**
	 * Tests if the TFunction is callable.
	 */
	FORCEINLINE explicit operator bool() const
	{
		return Super::IsSet();
	}
};

/**
 * TUniqueFunction<FuncType>
 *
 * Used like TFunction above, but is move-only.  This allows non-copyable functors to be bound to it.
 *
 * Example:
 *
 * TUniquePtr<FThing> Thing = MakeUnique<FThing>();
 *
 * TFunction      <void()> CopyableFunc = [Thing = MoveTemp(Thing)](){ Thing->DoSomething(); }; // error - lambda is not copyable
 * TUniqueFunction<void()> MovableFunc  = [Thing = MoveTemp(Thing)](){ Thing->DoSomething(); }; // ok
 *
 * void Foo(TUniqueFunction<void()> Func);
 * Foo(MovableFunc);           // error - TUniqueFunction is not copyable
 * Foo(MoveTemp(MovableFunc)); // ok
 */
template <typename FuncType>
class TUniqueFunction final : public UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<true>, FuncType>
{
	using Super = UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<true>, FuncType>;

public:
	/**
	 * Default constructor.
	 */
	TUniqueFunction(TYPE_OF_NULLPTR = nullptr)
	{
	}

	/**
	 * Constructor which binds a TFunction to any function object.
	 */
	template <
		typename FunctorType,
		typename = typename TEnableIf<
			TAnd<
				TNot<TOr<TIsTUniqueFunction<typename TDecay<FunctorType>::Type>, TIsTFunction<typename TDecay<FunctorType>::Type>>>,
				UE::Core::Private::Function::TFuncCanBindToFunctor<FuncType, FunctorType>
			>::Value
		>::Type
	>
	TUniqueFunction(FunctorType&& InFunc)
		: Super(Forward<FunctorType>(InFunc))
	{
		// This constructor is disabled for TUniqueFunction types so it isn't incorrectly selected as copy/move constructors.

		// This is probably a mistake if you expect TFunction to take a copy of what
		// TFunctionRef is bound to, because that's not possible.
		//
		// If you really intended to bind a TFunction to a TFunctionRef, you can just
		// wrap it in a lambda (and thus it's clear you're just binding to a call to another
		// reference):
		//
		// TFunction<int32(float)> MyFunction = [MyFunctionRef](float F) { return MyFunctionRef(F); };
		static_assert(!TIsTFunctionRef<typename TDecay<FunctorType>::Type>::Value, "Cannot construct a TUniqueFunction from a TFunctionRef");
	}

	/**
	 * Constructor which takes ownership of a TFunction's functor.
	 */
	TUniqueFunction(TFunction<FuncType>&& Other)
		: Super(MoveTemp(*(UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>*)&Other))
	{
	}

	/**
	 * Constructor which takes ownership of a TFunction's functor.
	 */
	TUniqueFunction(const TFunction<FuncType>& Other)
		: Super(*(const UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>*)&Other)
	{
	}

	/**
	 * Copy/move assignment operator.
	 */
	TUniqueFunction& operator=(TUniqueFunction&& Other)
	{
		Swap(*this, Other);
		return *this;
	}

	TUniqueFunction(TUniqueFunction&&) = default;
	TUniqueFunction(const TUniqueFunction& Other) = delete;
	TUniqueFunction& operator=(const TUniqueFunction& Other) = delete;
	~TUniqueFunction() = default;

	/**
	 * Removes any bound callable from the TFunction, restoring it to the default 'empty' state.
	 */
	void Reset()
	{
		*this = nullptr;
	}

	/**
	 * Tests if the TUniqueFunction is callable.
	 */
	FORCEINLINE explicit operator bool() const
	{
		return Super::IsSet();
	}
};

/**
 * Nullptr equality operator.
 */
template <typename FuncType>
FORCEINLINE bool operator==(TYPE_OF_NULLPTR, const TFunction<FuncType>& Func)
{
	return !Func;
}

/**
 * Nullptr equality operator.
 */
template <typename FuncType>
FORCEINLINE bool operator==(const TFunction<FuncType>& Func, TYPE_OF_NULLPTR)
{
	return !Func;
}

/**
 * Nullptr inequality operator.
 */
template <typename FuncType>
FORCEINLINE bool operator!=(TYPE_OF_NULLPTR, const TFunction<FuncType>& Func)
{
	return (bool)Func;
}

/**
 * Nullptr inequality operator.
 */
template <typename FuncType>
FORCEINLINE bool operator!=(const TFunction<FuncType>& Func, TYPE_OF_NULLPTR)
{
	return (bool)Func;
}
