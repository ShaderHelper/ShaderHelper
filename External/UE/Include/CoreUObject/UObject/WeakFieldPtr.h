// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WeakFieldPtr.h: Weak pointer to FField
=============================================================================*/

#pragma once

#include "UObject/WeakObjectPtrTemplates.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/FieldPath.h"

template<class T>
struct TWeakFieldPtr
{
private:

	// These exists only to disambiguate the two constructors below
	enum EDummy1 { Dummy1 };

	TWeakObjectPtr<UObject> Owner;
	mutable TFieldPath<T> Field;

public:
	TWeakFieldPtr() = default;
	TWeakFieldPtr(const TWeakFieldPtr&) = default;
	TWeakFieldPtr& operator=(const TWeakFieldPtr&) = default;
	~TWeakFieldPtr() = default;

	/**
	* Construct from a null pointer
	**/
	FORCEINLINE TWeakFieldPtr(TYPE_OF_NULLPTR)
		: Owner((UObject*)nullptr)
		, Field()
	{
	}

	/**
	* Construct from an object pointer
	* @param Object object to create a weak pointer to
	**/
	template <
		typename U,
		typename = decltype(ImplicitConv<T*>((U*)nullptr))
		>
		FORCEINLINE TWeakFieldPtr(U* InField, EDummy1 = Dummy1)
		: Owner(InField ? InField->GetOwnerUObject() : (UObject*)nullptr)
		, Field(InField)
	{
		// This static assert is in here rather than in the body of the class because we want
		// to be able to define TWeakFieldPtr<UUndefinedClass>.
		static_assert(TPointerIsConvertibleFromTo<T, const volatile FField>::Value, "TWeakFieldPtr can only be constructed with FField types");
	}

	/**
	* Construct from another weak pointer of another type, intended for derived-to-base conversions
	* @param Other weak pointer to copy from
	**/
	template <typename OtherT>
	FORCEINLINE TWeakFieldPtr(const TWeakFieldPtr<OtherT>& Other)
		: Owner(Other.Owner)
		, Field(Other.Field)
	{
		// It's also possible that this static_assert may fail for valid conversions because
		// one or both of the types have only been forward-declared.
		static_assert(TPointerIsConvertibleFromTo<OtherT, T>::Value, "Unable to convert TWeakFieldPtr - types are incompatible");
	}

	/**
	* Reset the weak pointer back to the NULL state
	*/
	FORCEINLINE void Reset()
	{
		Owner.Reset();
		Field.Reset();
	}

	/**
	* Copy from an object pointer
	* @param Object object to create a weak pointer to
	**/
	template<class U>
	FORCEINLINE typename TEnableIf<!TLosesQualifiersFromTo<U, T>::Value>::Type operator=(const U* InField)
	{
		Owner = InField ? InField->GetOwnerUObject() : (UObject*)nullptr;
		Field = (U*)InField;
	}

	/**
	* Assign from another weak pointer, intended for derived-to-base conversions
	* @param Other weak pointer to copy from
	**/
	template <typename OtherT>
	FORCEINLINE void operator=(const TWeakFieldPtr<OtherT>& Other)
	{
		// It's also possible that this static_assert may fail for valid conversions because
		// one or both of the types have only been forward-declared.
		static_assert(TPointerIsConvertibleFromTo<OtherT, T>::Value, "Unable to convert TWeakFieldPtr - types are incompatible");

		Owner = Other.Owner;
		Field = Other.Field;
	}

	/**
	* Dereference the weak pointer
	* @param bEvenIfPendingKill, if this is true, pendingkill objects are considered valid
	* @return NULL if this object is gone or the weak pointer was NULL, otherwise a valid uobject pointer
	**/
	FORCEINLINE T* Get(bool bEvenIfPendingKill) const
	{
		if (Owner.Get(bEvenIfPendingKill))
		{
			return Field.Get();
		}
		else
		{
			// Clear potentially stale pointer to the actual field
			Field.ClearCachedField();
		}
		return nullptr;
	}

	/**
	* Dereference the weak pointer. This is an optimized version implying bEvenIfPendingKill=false.
	*/
	FORCEINLINE T* Get(/*bool bEvenIfPendingKill = false*/) const
	{
		if (Owner.Get())
		{
			return Field.Get();
		}
		else
		{
			// Clear potentially stale pointer to the actual field
			Field.ClearCachedField();
		}
		return nullptr;
	}

	/** Deferences the weak pointer even if its marked RF_Unreachable. This is needed to resolve weak pointers during GC (such as ::AddReferenceObjects) */
	FORCEINLINE T* GetEvenIfUnreachable() const
	{
		if (Owner.GetEvenIfUnreachable())
		{
			return Field.Get();
		}
		else
		{
			// Clear potentially stale pointer to the actual field
			Field.ClearCachedField();
		}
		return nullptr;
	}

	/**
	* Dereference the weak pointer
	**/
	FORCEINLINE T & operator*() const
	{
		return *Get();
	}

	/**
	* Dereference the weak pointer
	**/
	FORCEINLINE T * operator->() const
	{
		return Get();
	}

	/**
	* Test if this points to a live FField
	* @param bEvenIfPendingKill, if this is true, pendingkill objects are considered valid
	* @param bThreadsafeTest, if true then function will just give you information whether referenced
	*							FField is gone forever (@return false) or if it is still there (@return true, no object flags checked).
	* @return true if Get() would return a valid non-null pointer
	**/
	FORCEINLINE bool IsValid(bool bEvenIfPendingKill, bool bThreadsafeTest = false) const
	{
		return Owner.IsValid(bEvenIfPendingKill, bThreadsafeTest) && Field.Get();
	}

	/**
	* Test if this points to a live FField. This is an optimized version implying bEvenIfPendingKill=false, bThreadsafeTest=false.
	* @return true if Get() would return a valid non-null pointer
	*/
	FORCEINLINE bool IsValid(/*bool bEvenIfPendingKill = false, bool bThreadsafeTest = false*/) const
	{
		return Owner.IsValid() && Field.Get();
	}

	/**
	* Slightly different than !IsValid(), returns true if this used to point to a FField, but doesn't any more and has not been assigned or reset in the mean time.
	* @param bIncludingIfPendingKill, if this is true, pendingkill objects are considered stale
	* @param bThreadsafeTest, set it to true when testing outside of Game Thread. Results in false if WeakObjPtr point to an existing object (no flags checked)
	* @return true if this used to point at a real object but no longer does.
	**/
	FORCEINLINE bool IsStale(bool bIncludingIfPendingKill = false, bool bThreadsafeTest = false) const
	{
		return Owner.IsStale(bIncludingIfPendingKill, bThreadsafeTest);
	}

	FORCEINLINE bool HasSameIndexAndSerialNumber(const TWeakFieldPtr& Other) const
	{
		return Owner.HasSameIndexAndSerialNumber(Other.Owner);
	}

	/** Hash function. */
	FORCEINLINE friend uint32 GetTypeHash(const TWeakFieldPtr& WeakObjectPtr)
	{
		return GetTypeHash(WeakObjectPtr.Field);
	}

	friend FArchive& operator<<(FArchive& Ar, TWeakFieldPtr& InWeakFieldPtr)
	{
		Ar << InWeakFieldPtr.Owner;
		Ar << InWeakFieldPtr.Field;
		return Ar;
	}

	/**
	* Compare weak pointers for equality
	* @param Other weak pointer to compare to
	**/
	template <typename TOther>
	FORCEINLINE bool operator==(const TWeakFieldPtr<TOther> &Other) const
	{
		static_assert(TPointerIsConvertibleFromTo<TOther, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
		static_assert(TPointerIsConvertibleFromTo<T, TOther>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

		return Field == Other.Field;
	}

	/**
	* Compare weak pointers for inequality
	* @param Other weak pointer to compare to
	**/
	template <typename TOther>
	FORCEINLINE bool operator!=(const TWeakFieldPtr<TOther> &Other) const
	{
		static_assert(TPointerIsConvertibleFromTo<TOther, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
		static_assert(TPointerIsConvertibleFromTo<T, TOther>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

		return Field != Other.Field;
	}

	/**
	* Compare weak pointers for equality
	* @param Other pointer to compare to
	**/
	template <typename TOther>
	FORCEINLINE bool operator==(const TOther* Other) const
	{
		static_assert(TPointerIsConvertibleFromTo<TOther, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
		static_assert(TPointerIsConvertibleFromTo<T, TOther>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

		return Field == Other;
	}

	/**
	* Compare weak pointers for inequality
	* @param Other pointer to compare to
	**/
	template <typename TOther>
	FORCEINLINE bool operator!=(const TOther* Other) const
	{
		static_assert(TPointerIsConvertibleFromTo<TOther, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
		static_assert(TPointerIsConvertibleFromTo<T, TOther>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

		return Field != Other;
	}
};

// Helper function which deduces the type of the initializer
template <typename T>
FORCEINLINE TWeakFieldPtr<T> MakeWeakFieldPtr(T* Ptr)
{
	return TWeakFieldPtr<T>(Ptr);
}

template <typename LhsT, typename RhsT>
FORCENOINLINE bool operator==(const LhsT* Lhs, const TWeakFieldPtr<RhsT>& Rhs)
{
	// It's also possible that these static_asserts may fail for valid conversions because
	// one or both of the types have only been forward-declared.
	static_assert(TPointerIsConvertibleFromTo<LhsT, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
	static_assert(TPointerIsConvertibleFromTo<LhsT, RhsT>::Value || TPointerIsConvertibleFromTo<RhsT, LhsT>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

	return Rhs == Lhs;
}

template <typename LhsT>
FORCENOINLINE bool operator==(const TWeakFieldPtr<LhsT>& Lhs, TYPE_OF_NULLPTR)
{
	return !Lhs.IsValid();
}

template <typename RhsT>
FORCENOINLINE bool operator==(TYPE_OF_NULLPTR, const TWeakFieldPtr<RhsT>& Rhs)
{
	return !Rhs.IsValid();
}

template <typename LhsT, typename RhsT>
FORCENOINLINE bool operator!=(const LhsT* Lhs, const TWeakFieldPtr<RhsT>& Rhs)
{
	// It's also possible that these static_asserts may fail for valid conversions because
	// one or both of the types have only been forward-declared.
	static_assert(TPointerIsConvertibleFromTo<LhsT, FField>::Value, "TWeakFieldPtr can only be compared with FField types");
	static_assert(TPointerIsConvertibleFromTo<LhsT, RhsT>::Value || TPointerIsConvertibleFromTo<RhsT, LhsT>::Value, "Unable to compare TWeakFieldPtr with raw pointer - types are incompatible");

	return Rhs != Lhs;
}

template <typename LhsT>
FORCENOINLINE bool operator!=(const TWeakFieldPtr<LhsT>& Lhs, TYPE_OF_NULLPTR)
{
	return Lhs.IsValid();
}

template <typename RhsT>
FORCENOINLINE bool operator!=(TYPE_OF_NULLPTR, const TWeakFieldPtr<RhsT>& Rhs)
{
	return Rhs.IsValid();
}

template<class T> struct TIsPODType<TWeakFieldPtr<T> > { enum { Value = true }; };
template<class T> struct TIsZeroConstructType<TWeakFieldPtr<T> > { enum { Value = true }; };
template<class T> struct TIsWeakPointerType<TWeakFieldPtr<T> > { enum { Value = true }; };


/**
* MapKeyFuncs for TWeakFieldPtrs which allow the key to become stale without invalidating the map.
*/
template <typename KeyType, typename ValueType, bool bInAllowDuplicateKeys = false>
struct TWeakFieldPtrMapKeyFuncs : public TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>
{
	typedef typename TDefaultMapKeyFuncs<KeyType, ValueType, bInAllowDuplicateKeys>::KeyInitType KeyInitType;

	static FORCEINLINE bool Matches(KeyInitType A, KeyInitType B)
	{
		return A == B;
	}

	static FORCEINLINE uint32 GetKeyHash(KeyInitType Key)
	{
		return GetTypeHash(Key);
	}
};