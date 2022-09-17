// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/IsPointer.h"
#include "HAL/UnrealMemory.h"
#include "Templates/EnableIf.h"
#include "Templates/AndOrNot.h"
#include "Templates/AreTypesEqual.h"
#include "Templates/CopyQualifiersAndRefsFromTo.h"
#include "Templates/IsArithmetic.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/RemoveReference.h"
#include "Templates/TypeCompatibleBytes.h"
#include "Templates/Identity.h"
#include "Traits/IsContiguousContainer.h"

/*-----------------------------------------------------------------------------
	Standard templates.
-----------------------------------------------------------------------------*/

/**
 * Chooses between the two parameters based on whether the first is nullptr or not.
 * @return If the first parameter provided is non-nullptr, it is returned; otherwise the second parameter is returned.
 */
template<typename ReferencedType>
FORCEINLINE ReferencedType* IfAThenAElseB(ReferencedType* A,ReferencedType* B)
{
	return A ? A : B;
}

/** branchless pointer selection based on predicate
* return PTRINT(Predicate) ? A : B;
**/
template<typename PredicateType,typename ReferencedType>
FORCEINLINE ReferencedType* IfPThenAElseB(PredicateType Predicate,ReferencedType* A,ReferencedType* B)
{
	return Predicate ? A : B;
}

/** A logical exclusive or function. */
inline bool XOR(bool A, bool B)
{
	return A != B;
}

/** This is used to provide type specific behavior for a copy which cannot change the value of B. */
template<typename T>
FORCEINLINE void Move(T& A,typename TMoveSupportTraits<T>::Copy B)
{
	// Destruct the previous value of A.
	A.~T();

	// Use placement new and a copy constructor so types with const members will work.
	new(&A) T(B);
}

/** This is used to provide type specific behavior for a move which may change the value of B. */
template<typename T>
FORCEINLINE void Move(T& A,typename TMoveSupportTraits<T>::Move B)
{
	// Destruct the previous value of A.
	A.~T();

	// Use placement new and a copy constructor so types with const members will work.
	new(&A) T(MoveTemp(B));
}

/**
 * Generically gets the data pointer of a contiguous container
 */
template<typename T, typename = typename TEnableIf<TIsContiguousContainer<T>::Value>::Type>
constexpr auto GetData(T&& Container) -> decltype(Container.GetData())
{
	return Container.GetData();
}

template <typename T, SIZE_T N> constexpr T* GetData(      T (& Container)[N]) { return Container; }
template <typename T, SIZE_T N> constexpr T* GetData(      T (&&Container)[N]) { return Container; }
template <typename T, SIZE_T N> constexpr const T* GetData(const T (& Container)[N]) { return Container; }
template <typename T, SIZE_T N> constexpr const T* GetData(const T (&&Container)[N]) { return Container; }

template <typename T>
constexpr const T* GetData(std::initializer_list<T> List)
{
	return List.begin();
}

/**
* Generically gets the number of items in a contiguous container
*/
template<typename T, typename = typename TEnableIf<TIsContiguousContainer<T>::Value>::Type>
constexpr auto GetNum(T&& Container) -> decltype(Container.Num())
{
	return Container.Num();
}

template <typename T, SIZE_T N> constexpr SIZE_T GetNum(      T (& Container)[N]) { return N; }
template <typename T, SIZE_T N> constexpr SIZE_T GetNum(      T (&&Container)[N]) { return N; }
template <typename T, SIZE_T N> constexpr SIZE_T GetNum(const T (& Container)[N]) { return N; }
template <typename T, SIZE_T N> constexpr SIZE_T GetNum(const T (&&Container)[N]) { return N; }

/**
 * Gets the number of items in an initializer list.
 *
 * The return type is int32 for compatibility with other code in the engine.
 * Realistically, an initializer list should not exceed the limits of int32.
 */
template <typename T>
constexpr int32 GetNum(std::initializer_list<T> List)
{
	return static_cast<int32>(List.size());
}

/**
 * Returns a non-const pointer type as const.
 */
template <typename T>
UE_DEPRECATED(4.26, "Call with a reference instead of a pointer.")
constexpr FORCEINLINE const T* AsConst(T* const& Ptr)
{
	return Ptr;
}

/**
 * Returns a non-const pointer type as const.
 */
template <typename T>
UE_DEPRECATED(4.26, "Call with a reference instead of a pointer.")
constexpr FORCEINLINE const T* AsConst(T* const&& Ptr)
{
	return Ptr;
}

/**
 * Returns a non-const reference type as const.
 */
template <typename T>
constexpr FORCEINLINE const T& AsConst(T& Ref)
{
	return Ref;
}

/**
 * Disallowed for rvalue references because it cannot extend their lifetime.
 */
template <typename T>
void AsConst(const T&& Ref) = delete;

/**
 * Returns a non-const reference type as const.
 * This overload is only required until the pointer overloads are removed.
 */
template <typename T, SIZE_T N>
constexpr FORCEINLINE const T (&AsConst(T (&Array)[N]))[N]
{
	return Array;
}

/** Test if value can make a lossless static_cast roundtrip via OutType without a sign change */
template<typename OutType, typename InType>
constexpr bool IntFitsIn(InType In)
{
	static_assert(std::is_integral_v<InType> && std::is_integral_v<OutType>, "Only integers supported");
	
	OutType Out = static_cast<OutType>(In);
	bool bRoundtrips = In == static_cast<InType>(Out);
	
	// Signed <-> unsigned cast requires sign test, signed -> smaller signed is covered by roundtrip sign-extension.
	if constexpr ((static_cast<InType>(-1) < InType{}) != (static_cast<OutType>(-1) < OutType{}))
	{
		return bRoundtrips && (In < InType{} == Out < OutType{});
	}
	
	return bRoundtrips;
}

/** Cast and check that value fits in OutType */
template<typename OutType, typename InType>
OutType IntCastChecked(InType In)
{
	check(IntFitsIn<OutType>(In));
	return static_cast<OutType>(In);
}

/*----------------------------------------------------------------------------
	Standard macros.
----------------------------------------------------------------------------*/

#ifdef __clang__
	template <typename T>
	auto UEArrayCountHelper(T& t) -> typename TEnableIf<__is_array(T), char(&)[sizeof(t) / sizeof(t[0]) + 1]>::Type;
#else
	template <typename T, uint32 N>
	char (&UEArrayCountHelper(const T (&)[N]))[N + 1];
#endif

// Number of elements in an array.
#define UE_ARRAY_COUNT( array ) (sizeof(UEArrayCountHelper(array)) - 1)
#define ARRAY_COUNT( array ) DEPRECATED_MACRO(4.24, "The ARRAY_COUNT macro has been deprecated in favor of UE_ARRAY_COUNT.") UE_ARRAY_COUNT( array )

// Offset of a struct member.
#ifndef UNREAL_CODE_ANALYZER
// UCA uses clang on Windows. According to C++11 standard, (which in this case clang follows and msvc doesn't)
// forbids using reinterpret_cast in constant expressions. msvc uses reinterpret_cast in offsetof macro,
// while clang uses compiler intrinsic. Calling static_assert(STRUCT_OFFSET(x, y) == SomeValue) causes compiler
// error when using clang on Windows (while including windows headers).
#define STRUCT_OFFSET( struc, member )	offsetof(struc, member)
#else
#define STRUCT_OFFSET( struc, member )	__builtin_offsetof(struc, member)
#endif

#if PLATFORM_VTABLE_AT_END_OF_CLASS
	#error need implementation
#else
	#define VTABLE_OFFSET( Class, MultipleInheritenceParent )	( ((PTRINT) static_cast<MultipleInheritenceParent*>((Class*)1)) - 1)
#endif


/**
 * works just like std::min_element.
 */
template<class ForwardIt> inline
ForwardIt MinElement(ForwardIt First, ForwardIt Last)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (*First < *Result) 
		{
			Result = First;
		}
	}
	return Result;
}

/**
 * works just like std::min_element.
 */
template<class ForwardIt, class PredicateType> inline
ForwardIt MinElement(ForwardIt First, ForwardIt Last, PredicateType Predicate)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (Predicate(*First,*Result))
		{
			Result = First;
		}
	}
	return Result;
}

/**
* works just like std::max_element.
*/
template<class ForwardIt> inline
ForwardIt MaxElement(ForwardIt First, ForwardIt Last)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (*Result < *First) 
		{
			Result = First;
		}
	}
	return Result;
}

/**
* works just like std::max_element.
*/
template<class ForwardIt, class PredicateType> inline
ForwardIt MaxElement(ForwardIt First, ForwardIt Last, PredicateType Predicate)
{
	ForwardIt Result = First;
	for (; ++First != Last; )
	{
		if (Predicate(*Result,*First))
		{
			Result = First;
		}
	}
	return Result;
}

/**
 * utility template for a class that should not be copyable.
 * Derive from this class to make your class non-copyable
 */
class FNoncopyable
{
protected:
	// ensure the class cannot be constructed directly
	FNoncopyable() {}
	// the class should not be used polymorphically
	~FNoncopyable() {}
private:
	FNoncopyable(const FNoncopyable&);
	FNoncopyable& operator=(const FNoncopyable&);
};

/** 
 * exception-safe guard around saving/restoring a value.
 * Commonly used to make sure a value is restored 
 * even if the code early outs in the future.
 * Usage:
 *  	TGuardValue<bool> GuardSomeBool(bSomeBool, false); // Sets bSomeBool to false, and restores it in dtor.
 */
template <typename RefType, typename AssignedType = RefType>
struct TGuardValue : private FNoncopyable
{
	TGuardValue(RefType& ReferenceValue, const AssignedType& NewValue)
	: RefValue(ReferenceValue), OldValue(ReferenceValue)
	{
		RefValue = NewValue;
	}
	~TGuardValue()
	{
		RefValue = OldValue;
	}

	/**
	 * Overloaded dereference operator.
	 * Provides read-only access to the original value of the data being tracked by this struct
	 *
	 * @return	a const reference to the original data value
	 */
	FORCEINLINE const AssignedType& operator*() const
	{
		return OldValue;
	}

private:
	RefType& RefValue;
	AssignedType OldValue;
};

template <typename FuncType>
struct TGuardValue_Bitfield_Cleanup : public FNoncopyable
{
	explicit TGuardValue_Bitfield_Cleanup(FuncType&& InFunc)
		: Func(MoveTemp(InFunc))
	{
	}

	~TGuardValue_Bitfield_Cleanup()
	{
		Func();
	}

private:
	FuncType Func;
};

/** 
 * Macro variant on TGuardValue<bool> that can deal with bitfields which cannot be passed by reference in to TGuardValue
 */
#define FGuardValue_Bitfield(ReferenceValue, NewValue) \
	const bool PREPROCESSOR_JOIN(TempBitfield, __LINE__) = ReferenceValue; \
	ReferenceValue = NewValue; \
	const TGuardValue_Bitfield_Cleanup<TFunction<void()>> PREPROCESSOR_JOIN(TempBitfieldCleanup, __LINE__)([&](){ ReferenceValue = PREPROCESSOR_JOIN(TempBitfield, __LINE__); });

/** 
 * Commonly used to make sure a value is incremented, and then decremented anyway the function can terminate.
 * Usage:
 *  	TScopeCounter<int32> BeginProcessing(ProcessingCount); // increments ProcessingCount, and decrements it in the dtor
 */
template <typename Type>
struct TScopeCounter : private FNoncopyable
{
	TScopeCounter(Type& ReferenceValue)
		: RefValue(ReferenceValue)
	{
		++RefValue;
	}
	~TScopeCounter()
	{
		--RefValue;
	}

private:
	Type& RefValue;
};


/**
 * Helper class to make it easy to use key/value pairs with a container.
 */
template <typename KeyType, typename ValueType>
struct TKeyValuePair
{
	TKeyValuePair( const KeyType& InKey, const ValueType& InValue )
	:	Key(InKey), Value(InValue)
	{
	}
	TKeyValuePair( const KeyType& InKey )
	:	Key(InKey)
	{
	}
	TKeyValuePair()
	{
	}
	bool operator==( const TKeyValuePair& Other ) const
	{
		return Key == Other.Key;
	}
	bool operator!=( const TKeyValuePair& Other ) const
	{
		return Key != Other.Key;
	}
	bool operator<( const TKeyValuePair& Other ) const
	{
		return Key < Other.Key;
	}
	FORCEINLINE bool operator()( const TKeyValuePair& A, const TKeyValuePair& B ) const
	{
		return A.Key < B.Key;
	}
	KeyType		Key;
	ValueType	Value;
};

//
// Macros that can be used to specify multiple template parameters in a macro parameter.
// This is necessary to prevent the macro parsing from interpreting the template parameter
// delimiting comma as a macro parameter delimiter.
// 

#define TEMPLATE_PARAMETERS2(X,Y) X,Y


/**
 * Removes one level of pointer from a type, e.g.:
 *
 * TRemovePointer<      int32  >::Type == int32
 * TRemovePointer<      int32* >::Type == int32
 * TRemovePointer<      int32**>::Type == int32*
 * TRemovePointer<const int32* >::Type == const int32
 */
template <typename T> struct TRemovePointer     { typedef T Type; };
template <typename T> struct TRemovePointer<T*> { typedef T Type; };

/**
 * MoveTemp will cast a reference to an rvalue reference.
 * This is UE's equivalent of std::move except that it will not compile when passed an rvalue or
 * const object, because we would prefer to be informed when MoveTemp will have no effect.
 */
template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj)
{
	typedef typename TRemoveReference<T>::Type CastType;

	// Validate that we're not being passed an rvalue or a const object - the former is redundant, the latter is almost certainly a mistake
	static_assert(TIsLValueReferenceType<T>::Value, "MoveTemp called on an rvalue");
	static_assert(!TAreTypesEqual<CastType&, const CastType&>::Value, "MoveTemp called on a const object");

	return (CastType&&)Obj;
}

/**
 * MoveTemp will cast a reference to an rvalue reference.
 * This is UE's equivalent of std::move.  It doesn't static assert like MoveTemp, because it is useful in
 * templates or macros where it's not obvious what the argument is, but you want to take advantage of move semantics
 * where you can but not stop compilation.
 */
template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTempIfPossible(T&& Obj)
{
	typedef typename TRemoveReference<T>::Type CastType;
	return (CastType&&)Obj;
}

/**
 * CopyTemp will enforce the creation of an rvalue which can bind to rvalue reference parameters.
 * Unlike MoveTemp, the source object will never be modifed. (i.e. a copy will be made)
 * There is no std:: equivalent.
 */
template <typename T>
FORCEINLINE T CopyTemp(T& Val)
{
	return const_cast<const T&>(Val);
}

template <typename T>
FORCEINLINE T CopyTemp(const T& Val)
{
	return Val;
}

template <typename T>
FORCEINLINE T&& CopyTemp(T&& Val)
{
	// If we already have an rvalue, just return it unchanged, rather than needlessly creating yet another rvalue from it.
	return MoveTemp(Val);
}

/**
 * Forward will cast a reference to an rvalue reference.
 * This is UE's equivalent of std::forward.
 */
template <typename T>
FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type& Obj)
{
	return (T&&)Obj;
}

template <typename T>
FORCEINLINE T&& Forward(typename TRemoveReference<T>::Type&& Obj)
{
	return (T&&)Obj;
}

/**
 * A traits class which specifies whether a Swap of a given type should swap the bits or use a traditional value-based swap.
 */
template <typename T>
struct TUseBitwiseSwap
{
	// We don't use bitwise swapping for 'register' types because this will force them into memory and be slower.
	enum { Value = !TOrValue<__is_enum(T), TIsPointer<T>, TIsArithmetic<T>>::Value };
};


/**
 * Swap two values.  Assumes the types are trivially relocatable.
 */
template <typename T>
inline typename TEnableIf<TUseBitwiseSwap<T>::Value>::Type Swap(T& A, T& B)
{
	if (LIKELY(&A != &B))
	{
		TTypeCompatibleBytes<T> Temp;
		FMemory::Memcpy(&Temp, &A, sizeof(T));
		FMemory::Memcpy(&A, &B, sizeof(T));
		FMemory::Memcpy(&B, &Temp, sizeof(T));
	}
}

template <typename T>
inline typename TEnableIf<!TUseBitwiseSwap<T>::Value>::Type Swap(T& A, T& B)
{
	T Temp = MoveTemp(A);
	A = MoveTemp(B);
	B = MoveTemp(Temp);
}

template <typename T>
inline void Exchange(T& A, T& B)
{
	Swap(A, B);
}

/**
 * This exists to avoid a Visual Studio bug where using a cast to forward an rvalue reference array argument
 * to a pointer parameter will cause bad code generation.  Wrapping the cast in a function causes the correct
 * code to be generated.
 */
template <typename T, typename ArgType>
FORCEINLINE T StaticCast(ArgType&& Arg)
{
	return static_cast<T>(Arg);
}

/**
 * TRValueToLValueReference converts any rvalue reference type into the equivalent lvalue reference, otherwise returns the same type.
 */
template <typename T> struct TRValueToLValueReference      { typedef T  Type; };
template <typename T> struct TRValueToLValueReference<T&&> { typedef T& Type; };

/**
 * Reverses the order of the bits of a value.
 * This is an TEnableIf'd template to ensure that no undesirable conversions occur.  Overloads for other types can be added in the same way.
 *
 * @param Bits - The value to bit-swap.
 * @return The bit-swapped value.
 */
template <typename T>
FORCEINLINE typename TEnableIf<TAreTypesEqual<T, uint32>::Value, T>::Type ReverseBits( T Bits )
{
	Bits = ( Bits << 16) | ( Bits >> 16);
	Bits = ( (Bits & 0x00ff00ff) << 8 ) | ( (Bits & 0xff00ff00) >> 8 );
	Bits = ( (Bits & 0x0f0f0f0f) << 4 ) | ( (Bits & 0xf0f0f0f0) >> 4 );
	Bits = ( (Bits & 0x33333333) << 2 ) | ( (Bits & 0xcccccccc) >> 2 );
	Bits = ( (Bits & 0x55555555) << 1 ) | ( (Bits & 0xaaaaaaaa) >> 1 );
	return Bits;
}

/**
 * Generates a bitmask with a given number of bits set.
 */
template <typename T>
FORCEINLINE T BitMask( uint32 Count );

template <>
FORCEINLINE uint64 BitMask<uint64>( uint32 Count )
{
	checkSlow(Count <= 64);
	return (uint64(Count < 64) << Count) - 1;
}

template <>
FORCEINLINE uint32 BitMask<uint32>( uint32 Count )
{
	checkSlow(Count <= 32);
	return uint32(uint64(1) << Count) - 1;
}

template <>
FORCEINLINE uint16 BitMask<uint16>( uint32 Count )
{
	checkSlow(Count <= 16);
	return uint16((uint32(1) << Count) - 1);
}

template <>
FORCEINLINE uint8 BitMask<uint8>( uint32 Count )
{
	checkSlow(Count <= 8);
	return uint8((uint32(1) << Count) - 1);
}


/** Template for initializing a singleton at the boot. */
template< class T >
struct TForceInitAtBoot
{
	TForceInitAtBoot()
	{
		T::Get();
	}
};

/** Used to avoid cluttering code with ifdefs. */
struct FNoopStruct
{
	FNoopStruct()
	{}

	~FNoopStruct()
	{}
};

/**
 * Equivalent to std::declval.
 *
 * Note that this function is unimplemented, and is only intended to be used in unevaluated contexts, like sizeof and trait expressions.
 */
template <typename T>
T&& DeclVal();

/**
 * Uses implicit conversion to create an instance of a specific type.
 * Useful to make things clearer or circumvent unintended type deduction in templates.
 * Safer than C casts and static_casts, e.g. does not allow down-casts
 *
 * @param Obj  The object (usually pointer or reference) to convert.
 *
 * @return The object converted to the specified type.
 */
template <typename T>
FORCEINLINE T ImplicitConv(typename TIdentity<T>::Type Obj)
{
    return Obj;
}

/**
 * ForwardAsBase will cast a reference to an rvalue reference of a base type.
 * This allows the perfect forwarding of a reference as a base class.
 */
template <
	typename T,
	typename Base,
	decltype(ImplicitConv<const volatile Base*>((typename TRemoveReference<T>::Type*)nullptr))* = nullptr
>
FORCEINLINE decltype(auto) ForwardAsBase(typename TRemoveReference<T>::Type& Obj)
{
	return (TCopyQualifiersAndRefsFromTo_T<T&&, Base>)Obj;
}

template <
	typename T,
	typename Base,
	decltype(ImplicitConv<const volatile Base*>((typename TRemoveReference<T>::Type*)nullptr))* = nullptr
>
FORCEINLINE decltype(auto) ForwardAsBase(typename TRemoveReference<T>::Type&& Obj)
{
	return (TCopyQualifiersAndRefsFromTo_T<T&&, Base>)Obj;
}
