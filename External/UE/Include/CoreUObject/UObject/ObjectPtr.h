// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"
#include "Serialization/StructuredArchive.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/ObjectHandle.h"

#include <type_traits>

#define UE_WITH_OBJECT_PTR_DEPRECATIONS 0
#if UE_WITH_OBJECT_PTR_DEPRECATIONS
	#define UE_OBJPTR_DEPRECATED(Version, Message) UE_DEPRECATED(Version, Message)
#else
	#define UE_OBJPTR_DEPRECATED(Version, Message) 
#endif

/** 
 * Wrapper macro for use in places where code needs to allow for a pointer type that could be a TObjectPtr<T> or a raw object pointer during a transitional period.
 * The coding standard disallows general use of the auto keyword, but in wrapping it in this macro, we have a record
 * of the explicit type meant to be used, and an avenue to go back and change these instances to an explicit
 * TObjectPtr<T> after the transition is complete and the type won't be toggling back and forth anymore.
 */
#define UE_TRANSITIONAL_OBJECT_PTR(Type) auto 

#if PLATFORM_MICROSOFT && defined(_MSC_EXTENSIONS)
	/**
	 * Non-conformance mode in MSVC has issues where the presence of a conversion operator to bool (even an explicit one)
	 * leads to operator ambiguity for operator== and operator!= with a NULL (not nullptr).  This macro controls the presence
	 * of code to deal with this issue.
	 */
	#define UE_OBJECT_PTR_NONCONFORMANCE_SUPPORT 1
#else
	#define UE_OBJECT_PTR_NONCONFORMANCE_SUPPORT 0
#endif

template <typename T>
struct TObjectPtr;

/**
 * FObjectPtr is the basic, minimally typed version of TObjectPtr
 */
struct FObjectPtr
{
public:
	FObjectPtr() = default;

	FORCEINLINE FObjectPtr(TYPE_OF_NULLPTR)
		: Handle(MakeObjectHandle(nullptr))
	{
	}

	explicit FORCEINLINE FObjectPtr(UObject* Object)
		: Handle(MakeObjectHandle(Object))
	{
	}

	UE_OBJPTR_DEPRECATED(5.0, "Construction with incomplete type pointer is deprecated.  Please update this code to use MakeObjectPtrUnsafe.")
	explicit FORCEINLINE FObjectPtr(void* IncompleteObject)
		: Handle(MakeObjectHandle(reinterpret_cast<UObject*>(IncompleteObject)))
	{
	}

	explicit FORCEINLINE FObjectPtr(const FObjectRef& ObjectRef)
		: Handle(MakeObjectHandle(ObjectRef))
	{
	}

	explicit FORCEINLINE FObjectPtr(const FPackedObjectRef& PackedObjectRef)
		: Handle(MakeObjectHandle(PackedObjectRef))
	{
	}

	FORCEINLINE UObject* Get() const
	{
		return ResolveObjectHandle(Handle);
	}

	FORCEINLINE UClass* GetClass() const
	{
		return ResolveObjectHandleClass(Handle);
	}

	FObjectPtr(FObjectPtr&&) = default;
	FObjectPtr(const FObjectPtr&) = default;
	FObjectPtr& operator=(FObjectPtr&&) = default;
	FObjectPtr& operator=(const FObjectPtr&) = default;

	FObjectPtr& operator=(UObject* Other)
	{
		Handle = MakeObjectHandle(Other);
		return *this;
	}

	UE_OBJPTR_DEPRECATED(5.0, "Assignment with incomplete type pointer is deprecated.  Please update this code to use MakeObjectPtrUnsafe.")
	FObjectPtr& operator=(void* IncompleteOther)
	{
		Handle = MakeObjectHandle(reinterpret_cast<UObject*>(IncompleteOther));
		return *this;
	}

	FObjectPtr& operator=(TYPE_OF_NULLPTR)
	{
		Handle = MakeObjectHandle(nullptr);
		return *this;
	}

	FORCEINLINE bool operator==(FObjectPtr Other) const { return (Handle == Other.Handle); }
	FORCEINLINE bool operator!=(FObjectPtr Other) const { return (Handle != Other.Handle); }

	UE_OBJPTR_DEPRECATED(5.0, "Use of ToTObjectPtr is unsafe and is deprecated.")
	FORCEINLINE TObjectPtr<UObject>& ToTObjectPtr();
	UE_OBJPTR_DEPRECATED(5.0, "Use of ToTObjectPtr is unsafe and is deprecated.")
	FORCEINLINE const TObjectPtr<UObject>& ToTObjectPtr() const;

	FORCEINLINE UObject* operator->() const { return Get(); }
	FORCEINLINE UObject& operator*() const { return *Get(); }

	// Forcing resolve for null check.  Non null unresolved pointers can become null resolved pointers.
	FORCEINLINE bool IsNull() const { return ResolveObjectHandleNoRead(Handle) == nullptr; }
	FORCEINLINE bool IsNullNoResolve() const { return IsObjectHandleNull(Handle); }

	FORCEINLINE bool operator!() const { return IsNull(); }
	explicit FORCEINLINE operator bool() const { return !IsNull(); }

	FORCEINLINE bool IsResolved() const { return IsObjectHandleResolved(Handle); }

	COREUOBJECT_API FString GetPath() const;

	FORCEINLINE FObjectHandle GetHandle() const { return Handle; }
	FORCEINLINE FObjectHandle& GetHandleRef() const { return Handle; }

	FORCEINLINE bool IsA(const UClass* SomeBase) const
	{
		checkfSlow(SomeBase, TEXT("IsA(NULL) cannot yield meaningful results"));

		const UClass* ThisClass = GetClass();
		checkfSlow(ThisClass, TEXT("Object pointer class did not resolve to a non-null value"));

		return ThisClass->IsChildOf(SomeBase);
	}

	template <typename T>
	FORCEINLINE bool IsA() const
	{
		return IsA(T::StaticClass());
	}

protected:
	FORCEINLINE UObject* GetNoRead() const
	{
		return ResolveObjectHandleNoRead(Handle);
	}

private:
	friend FORCEINLINE uint32 GetTypeHash(const FObjectPtr& Object)
	{
		Object.Get();
		return GetTypeHash(Object.Handle);
	}

	mutable FObjectHandle Handle;
};

template <typename T>
struct TPrivateObjectPtr;

template <typename T>
struct TIsTObjectPtr
{
	enum { Value = false };
};

namespace ObjectPtr_Private
{
	template <typename T, typename U>
	FORCEINLINE bool IsObjectPtrEqualToRawPtrOfRelatedType(const TObjectPtr<T>& Ptr, const U* Other);
};

/**
 * TObjectPtr is a type of pointer to a UObject that is meant to function as a drop-in replacement for raw pointer
 * member properties. It is size equivalent to a 64-bit pointer and supports access tracking and optional lazy load
 * behavior in editor builds. It stores either the address to the referenced object or (in editor builds) an index in
 * the object handle table that describes a referenced object that hasn't been loaded yet. It is serialized
 * identically to a raw pointer to a UObject. When resolved, its participation in garbage collection is identical to a
 * raw pointer to a UObject.
 *
 * This is useful for automatic replacement of raw pointers to support advanced cook-time dependency tracking and
 * editor-time lazy load use cases. See UnrealObjectPtrTool for tooling to automatically replace raw pointer members
 * with FObjectPtr/TObjectPtr members instead.
 */
template <typename T>
struct TObjectPtr : private FObjectPtr
{
public:
	using ElementType = T;

	TObjectPtr() = default;
	TObjectPtr(TObjectPtr<T>&&) = default;
	TObjectPtr(const TObjectPtr<T>&) = default;

	FORCEINLINE TObjectPtr(TYPE_OF_NULLPTR)
		: FObjectPtr(nullptr)
	{
	}

	template <
		typename U,
		decltype(ImplicitConv<T*>(std::declval<U*>()))* = nullptr
	>
	FORCEINLINE TObjectPtr(const TObjectPtr<U>& Other)
		: FObjectPtr(static_cast<const FObjectPtr&>(Other))
	{
	}

	template <
		typename U,
		std::enable_if_t<
			!TIsTObjectPtr<std::decay_t<U>>::Value,
			decltype(ImplicitConv<T*>(std::declval<U>()))
		>* = nullptr
	>
	FORCEINLINE TObjectPtr(U&& Object)
		: FObjectPtr(const_cast<std::remove_const_t<T>*>(ImplicitConv<T*>(Object)))
	{
	}

	explicit FORCEINLINE TObjectPtr(TPrivateObjectPtr<T>&& PrivatePtr)
		: FObjectPtr(const_cast<UObject*>(PrivatePtr.Pointer))
	{
	}

	TObjectPtr<T>& operator=(TObjectPtr<T>&&) = default;
	TObjectPtr<T>& operator=(const TObjectPtr<T>&) = default;

	FORCEINLINE TObjectPtr<T>& operator=(TYPE_OF_NULLPTR)
	{
		FObjectPtr::operator=(nullptr);
		return *this;
	}

	template <
		typename U,
		decltype(ImplicitConv<T*>(std::declval<U*>()))* = nullptr
	>
	FORCEINLINE TObjectPtr<T>& operator=(const TObjectPtr<U>& Other)
	{
		FObjectPtr::operator=(static_cast<const FObjectPtr&>(Other));
		return *this;
	}

	template <
		typename U,
		std::enable_if_t<
			!TIsTObjectPtr<std::decay_t<U>>::Value,
			decltype(ImplicitConv<T*>(std::declval<U>()))
		>* = nullptr
	>
	FORCEINLINE TObjectPtr<T>& operator=(U&& Object)
	{
		FObjectPtr::operator=(const_cast<std::remove_const_t<T>*>(ImplicitConv<T*>(Object)));
		return *this;
	}

	FORCEINLINE TObjectPtr<T>& operator=(TPrivateObjectPtr<T>&& PrivatePtr)
	{
		FObjectPtr::operator=(const_cast<UObject*>(PrivatePtr.Pointer));
		return *this;
	}

	template <
		typename U,
		typename Base = std::decay_t<decltype(false ? std::declval<std::decay_t<T*>>() : std::declval<std::decay_t<U*>>())>
	>
	FORCEINLINE bool operator==(const TObjectPtr<U>& Other) const
	{
		return FObjectPtr::operator==(static_cast<const FObjectPtr&>(Other));
	}

	template <
		typename U,
		typename Base = std::decay_t<decltype(false ? std::declval<std::decay_t<T*>>() : std::declval<std::decay_t<U*>>())>
	>
	FORCEINLINE bool operator!=(const TObjectPtr<U>& Other) const
	{
		return FObjectPtr::operator!=(static_cast<const FObjectPtr&>(Other));
	}

	// @TODO: OBJPTR: There is a risk that the FObjectPtr is storing a reference to the wrong type.  This could
	//			happen if data was serialized at a time when a pointer field was declared to be of type A, but then the declaration
	//			changed and the pointer field is now of type B.  Upon deserialization of pre-existing data, we'll be holding
	//			a reference to the wrong type of object which we'll just send back static_casted as the wrong type.  Doing
	//			a check or checkSlow here could catch this, but it would be better if the check could happen elsewhere that
	//			isn't called as frequently.
	FORCEINLINE T* Get() const { return (T*)(FObjectPtr::Get()); }
	using FObjectPtr::GetClass;
	
	FORCEINLINE operator T* () const { return Get(); }
	template <typename U>
	UE_OBJPTR_DEPRECATED(5.0, "Explicit cast to other raw pointer types is deprecated.  Please use the Cast API or get the raw pointer with ToRawPtr and cast that instead.")
	explicit FORCEINLINE operator U* () const { return (U*)Get(); }
	explicit FORCEINLINE operator UPTRINT() const { return (UPTRINT)Get(); }
	FORCEINLINE T* operator->() const { return Get(); }
	FORCEINLINE T& operator*() const { return *Get(); }

	UE_OBJPTR_DEPRECATED(5.0, "Conversion to a mutable pointer is deprecated.  Please pass a TObjectPtr<T>& instead so that assignment can be tracked accurately.")
	explicit FORCEINLINE operator T*& () { return GetInternalRef(); }

	using FObjectPtr::IsNull;
	using FObjectPtr::IsNullNoResolve;
	using FObjectPtr::operator!;
	using FObjectPtr::operator bool;
	using FObjectPtr::IsResolved;
	using FObjectPtr::GetPath;
	using FObjectPtr::GetHandle;

	using FObjectPtr::IsA;

	friend FORCEINLINE uint32 GetTypeHash(const TObjectPtr<T>& ObjectPtr)
	{
		return GetTypeHash(static_cast<const FObjectPtr&>(ObjectPtr));
	}

	friend FORCEINLINE FArchive& operator<<(FArchive& Ar, TObjectPtr<T>& ObjectPtr)
	{
		Ar << static_cast<FObjectPtr&>(ObjectPtr);
		return Ar;
	}

	friend FORCEINLINE void operator<<(FStructuredArchiveSlot&& Slot, TObjectPtr<T>& ObjectPtr)
	{
		Slot << static_cast<FObjectPtr&>(ObjectPtr);
	}

	friend struct FObjectPtr;
	template <typename U> friend struct TObjectPtr;
	template <typename U, typename V> friend bool ObjectPtr_Private::IsObjectPtrEqualToRawPtrOfRelatedType(const TObjectPtr<U>& Ptr, const V* Other);
private:

	FORCEINLINE T* GetNoRead() const { return (T*)(FObjectPtr::GetNoRead()); }

	// @TODO: OBJPTR: There is a risk of a gap in access tracking here.  The caller may get a mutable pointer, write to it, then
	//			read from it.  That last read would happen without an access being recorded.  Not sure if there is a good way
	//			to handle this case without forcing the calling code to be modified.
	FORCEINLINE T*& GetInternalRef()
	{
		FObjectPtr::Get();
		return (T*&)FObjectPtr::GetHandleRef();
	}
};

template <typename T> struct TIsTObjectPtr<               TObjectPtr<T>> { enum { Value = true }; };
template <typename T> struct TIsTObjectPtr<const          TObjectPtr<T>> { enum { Value = true }; };
template <typename T> struct TIsTObjectPtr<      volatile TObjectPtr<T>> { enum { Value = true }; };
template <typename T> struct TIsTObjectPtr<const volatile TObjectPtr<T>> { enum { Value = true }; };

template <typename T>
struct TRemoveObjectPointer
{
	typedef T Type;
};
template <typename T>
struct TRemoveObjectPointer<TObjectPtr<T>>
{
	typedef T Type;
};

namespace ObjectPtr_Private
{
	template <typename T> struct TRawPointerType                               { using Type = T;  };
	template <typename T> struct TRawPointerType<               TObjectPtr<T>> { using Type = T*; };
	template <typename T> struct TRawPointerType<const          TObjectPtr<T>> { using Type = T*; };
	template <typename T> struct TRawPointerType<      volatile TObjectPtr<T>> { using Type = T*; };
	template <typename T> struct TRawPointerType<const volatile TObjectPtr<T>> { using Type = T*; };

	/** Coerce to pointer through implicit conversion to const T* (overload through less specific "const T*" parameter to avoid ambiguity with other coercion options that may also exist. */
	template <typename T>
	FORCEINLINE const T* CoerceToPointer(const T* Other)
	{
		return Other;
	}

#if UE_OBJECT_PTR_NONCONFORMANCE_SUPPORT
	/**
	 * Force acceptance of explicitly TYPE_OF_NULL to avoid ambiguity issues triggered by the presence of a type conversion operator to bool.
	 * This has the negative consequence of making it possible to coerce an arbitrary integer for the purpose of use in the comparison operators.
	 */
	template <typename T>
	UE_OBJPTR_DEPRECATED(5.0, "Coercing a NULL for operations with a TObjectPtr is deprecated when running in a non-standards conforming compiler mode.")
	constexpr const T* CoerceToPointer(TYPE_OF_NULL Other)
	{
		checkfSlow(Other == 0, TEXT("TObjectPtr cannot be compared to a non-zero NULL type value."));
		return nullptr;
	}
#endif

	/** Coerce to pointer through implicit conversion to CommonPointerType where CommonPointerType is deduced, and must be a C++ pointer, not a wrapper type. */
	template <
		typename T,
		typename U,
		typename CommonPointerType =  decltype(std::declval<bool>() ? std::declval<const T*>() : std::declval<U>()),
		std::enable_if_t<
			std::is_pointer<CommonPointerType>::value
		>* = nullptr
	>
	FORCEINLINE auto CoerceToPointer(U&& Other) -> CommonPointerType
	{
		return Other;
	}

	/** Coerce to pointer through the use of a ".Get()" member, which is the convention within Unreal smart pointer types. */
	template <
		typename T,
		typename U,
		std::enable_if_t<
			!TIsTObjectPtr<std::decay_t<U>>::Value,
			decltype(std::declval<U>().Get())
		>* = nullptr
	>
	FORCEINLINE auto CoerceToPointer(U&& Other) -> decltype(std::declval<U>().Get())
	{
		return Other.Get();
	}

	template <typename T, typename U>
	FORCEINLINE bool IsObjectPtrEqualToRawPtrOfRelatedType(const TObjectPtr<T>& Ptr, const U* Other)
	{
		return Ptr.GetNoRead() == Other;
	}

	/** Perform shallow equality check between a TObjectPtr and another (non TObjectPtr) type that we can coerce to a pointer. */
	template <
		typename T,
		typename U,
		std::enable_if_t<
			!TIsTObjectPtr<std::decay_t<U>>::Value,
			decltype(CoerceToPointer<T>(std::declval<U>()) == std::declval<const T*>())
		>* = nullptr
	>
	FORCEINLINE bool IsObjectPtrEqual(const TObjectPtr<T>& Ptr, U&& Other)
	{
		// This function deliberately avoids the tracking code path as we are only doing
		// a shallow pointer comparison.
		return IsObjectPtrEqualToRawPtrOfRelatedType<T>(Ptr, ObjectPtr_Private::CoerceToPointer<T>(Other));
	}
}

// Equality/Inequality comparisons against another type that can be implicitly converted to the pointer type kept in a TObjectPtr
template <
	typename T,
	typename U,
	decltype(ObjectPtr_Private::IsObjectPtrEqual(std::declval<const TObjectPtr<T>&>(), std::declval<U&&>()))* = nullptr
>
FORCEINLINE bool operator==(const TObjectPtr<T>& Ptr, U&& Other)
{
	return ObjectPtr_Private::IsObjectPtrEqual(Ptr, Other);
}
template <
	typename T,
	typename U,
	decltype(ObjectPtr_Private::IsObjectPtrEqual(std::declval<const TObjectPtr<T>&>(), std::declval<U&&>()))* = nullptr
>
FORCEINLINE bool operator==(U&& Other, const TObjectPtr<T>& Ptr)
{
	return ObjectPtr_Private::IsObjectPtrEqual(Ptr, Other);
}
template <
	typename T,
	typename U,
	decltype(ObjectPtr_Private::IsObjectPtrEqual(std::declval<const TObjectPtr<T>&>(), std::declval<U&&>()))* = nullptr
>
FORCEINLINE bool operator!=(const TObjectPtr<T>& Ptr, U&& Other)
{
	return !ObjectPtr_Private::IsObjectPtrEqual(Ptr, Other);
}
template <
	typename T,
	typename U,
	decltype(ObjectPtr_Private::IsObjectPtrEqual(std::declval<const TObjectPtr<T>&>(), std::declval<U&&>()))* = nullptr
>
FORCEINLINE bool operator!=(U&& Other, const TObjectPtr<T>& Ptr)
{
	return !ObjectPtr_Private::IsObjectPtrEqual(Ptr, Other);
}

template <typename T>
TPrivateObjectPtr<T> MakeObjectPtrUnsafe(const UObject* Obj);

template <typename T>
struct TPrivateObjectPtr
{
public:
	TPrivateObjectPtr(const TPrivateObjectPtr<T>& Other) = default;

private:
	/** Only for use by MakeObjectPtrUnsafe */
	explicit TPrivateObjectPtr(const UObject* InPointer)
		: Pointer(InPointer)
	{
	}

	const UObject* Pointer;
	friend struct TObjectPtr<T>;
	friend TPrivateObjectPtr MakeObjectPtrUnsafe<T>(const UObject* Obj);
};

/** Used to allow the caller to provide a pointer to an incomplete type of T that has explicitly cast to a UObject. */
template <typename T>
TPrivateObjectPtr<T> MakeObjectPtrUnsafe(const UObject* Obj)
{
	return TPrivateObjectPtr<T>{Obj};
}

template <typename T>
FORCEINLINE T* ToRawPtr(const TObjectPtr<T>& Ptr)
{
	// NOTE: This is specifically not getting a reference to the internal pointer.
	return Ptr.Get();
}

template <typename T>
FORCEINLINE T* ToRawPtr(T* Ptr)
{
	return Ptr;
}

template <typename T, SIZE_T Size>
FORCEINLINE T** ToRawPtrArrayUnsafe(TObjectPtr<T>(&ArrayOfPtr)[Size])
{
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE || UE_WITH_OBJECT_HANDLE_TRACKING
	for (TObjectPtr<T>& Item : ArrayOfPtr)
	{
		// NOTE: Relying on the fact that the TObjectPtr will cache the resolved pointer in place after calling Get.
		Item.Get();
	}
#endif
	return reinterpret_cast<T**>(ArrayOfPtr);
}

template <typename T>
FORCEINLINE T** ToRawPtrArrayUnsafe(T** ArrayOfPtr)
{
	return ArrayOfPtr;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
template <
	typename ArrayType,
	typename ArrayTypeNoRef = std::remove_reference_t<ArrayType>,
	std::enable_if_t<TIsTArray<ArrayTypeNoRef>::Value>* = nullptr
>
decltype(auto) ToRawPtrTArrayUnsafe(ArrayType&& Array)
{
	using ArrayElementType         = typename ArrayTypeNoRef::ElementType;
	using ArrayAllocatorType       = typename ArrayTypeNoRef::AllocatorType;
	using RawPointerType           = typename ObjectPtr_Private::TRawPointerType<ArrayElementType>::Type;
	using QualifiedRawPointerType  = typename TCopyQualifiersFromTo<ArrayElementType, RawPointerType>::Type;
	using NewArrayType             = TArray<QualifiedRawPointerType, ArrayAllocatorType>;
	using RefQualifiedNewArrayType = typename TCopyQualifiersAndRefsFromTo<ArrayType, NewArrayType>::Type;

	return (RefQualifiedNewArrayType&)Array;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

template <typename T>
struct TContainerElementTypeCompatibility<TObjectPtr<T>>
{
	typedef T* ReinterpretType;
	
	template <typename IterBeginType, typename IterEndType, typename OperatorType = std::remove_reference_t<decltype(*std::declval<IterBeginType>())>& (*)(IterBeginType&)>
	UE_OBJPTR_DEPRECATED(5.0, "Reinterpretation between ranges of one type to another type is deprecated.")
	static void ReinterpretRange(IterBeginType Iter, IterEndType IterEnd, OperatorType Operator = [](IterBeginType& InIt) -> decltype(auto) { return *InIt; })
	{
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE || UE_WITH_OBJECT_HANDLE_TRACKING
		while (Iter != IterEnd)
		{
			Operator(Iter).Get();
			++Iter;
		}
#endif
	}

	typedef T* CopyFromOtherType;

	UE_OBJPTR_DEPRECATED(5.0, "Copying ranges of one type to another type is deprecated.")
	static constexpr void CopyingFromOtherType() {}
};

template <typename T>
struct TContainerElementTypeCompatibility<const TObjectPtr<T>>
{
	typedef T* const ReinterpretType;

	template <typename IterBeginType, typename IterEndType, typename OperatorType = const std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<IterBeginType>())>>&(*)(IterBeginType&)>
	UE_OBJPTR_DEPRECATED(5.0, "Reinterpretation between ranges of one type to another type is deprecated.")
	static void ReinterpretRange(IterBeginType Iter, IterEndType IterEnd, OperatorType Operator = [](IterBeginType& InIt) -> decltype(auto) { return *InIt; })
	{
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE || UE_WITH_OBJECT_HANDLE_TRACKING
		while (Iter != IterEnd)
		{
			Operator(Iter).Get();
			++Iter;
		}
#endif
	}

	typedef T* const CopyFromOtherType;

	UE_OBJPTR_DEPRECATED(5.0, "Copying ranges of one type to another type is deprecated.")
	static constexpr void CopyingFromOtherType() {}
};


// Trait which allows TObjectPtr to be default constructed by memsetting to zero.
template <typename T>
struct TIsZeroConstructType<TObjectPtr<T>>
{
	enum { Value = true };
};

// Trait which allows TObjectPtr to be memcpy'able from pointers.
template <typename T>
struct TIsBitwiseConstructible<TObjectPtr<T>, T*>
{
	enum { Value = true };
};

template <typename T, class PREDICATE_CLASS>
struct TDereferenceWrapper<TObjectPtr<T>, PREDICATE_CLASS>
{
	const PREDICATE_CLASS& Predicate;

	TDereferenceWrapper(const PREDICATE_CLASS& InPredicate)
		: Predicate(InPredicate) {}

	/** Dereference pointers */
	FORCEINLINE bool operator()(const TObjectPtr<T>& A, const TObjectPtr<T>& B) const
	{
		return Predicate(*A, *B);
	}
};

template <typename T>
struct TCallTraits<TObjectPtr<T>> : public TCallTraitsBase<TObjectPtr<T>>
{
	using ConstPointerType = typename TCallTraitsParamTypeHelper<const TObjectPtr<const T>, true>::ConstParamType;
};


template <typename T>
FORCEINLINE TWeakObjectPtr<T> MakeWeakObjectPtr(TObjectPtr<T> Ptr)
{
	return TWeakObjectPtr<T>(Ptr);
}

FORCEINLINE TObjectPtr<UObject>& FObjectPtr::ToTObjectPtr()
{
	return *static_cast<TObjectPtr<UObject>*>(this);
}

FORCEINLINE const TObjectPtr<UObject>& FObjectPtr::ToTObjectPtr() const
{
	return *static_cast<const TObjectPtr<UObject>*>(this);
}

/** Swap variants between TObjectPtr<T> and raw pointer to T */
template <typename T>
inline void Swap(TObjectPtr<T>& A, T*& B)
{
	Swap((T*&)A, B);
}
template <typename T>
inline void Swap(T*& A, TObjectPtr<T>& B)
{
	Swap(A, (T*&)B);
}

/** Swap variants between TArray<TObjectPtr<T>> and TArray<T*> */
template <typename T>
inline void Swap(TArray<TObjectPtr<T>>& A, TArray<T*>& B)
{
	Swap(ToRawPtrTArrayUnsafe(A), B);
}
template <typename T>
inline void Swap(TArray<T*>& A, TArray<TObjectPtr<T>>& B)
{
	Swap(A, ToRawPtrTArrayUnsafe(B));
}

/** Exchange variants between TObjectPtr<T> and raw pointer to T */
template <typename T>
inline void Exchange(TObjectPtr<T>& A, T*& B)
{
	Swap((T*&)A, B);
}
template <typename T>
inline void Exchange(T*& A, TObjectPtr<T>& B)
{
	Swap(A, (T*&)B);
}

/** Exchange variants between TArray<TObjectPtr<T>> and TArray<T*> */
template <typename T>
inline void Exchange(TArray<TObjectPtr<T>>& A, TArray<T*>& B)
{
	Swap(ToRawPtrTArrayUnsafe(A), B);
}
template <typename T>
inline void Exchange(TArray<T*>& A, TArray<TObjectPtr<T>>& B)
{
	Swap(A, ToRawPtrTArrayUnsafe(B));
}
