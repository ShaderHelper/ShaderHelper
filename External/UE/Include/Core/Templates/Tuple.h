// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/AreTypesEqual.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/Decay.h"
#include "Delegates/IntegerSequence.h"
#include "Templates/Invoke.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/MemoryLayout.h"
#include "Templates/TypeHash.h"
#include "Templates/IsConstructible.h"
#include <tuple>

// This workaround exists because Visual Studio causes false positives for code like this during static analysis:
//
// void TestFoo(TMap<int*, int>& Map)
// {
//     for (const TPair<int*, int>& Pair : Map)
//     {
//         if (*Pair.Key == 15 && Pair.Value != 15) // warning C6011: Dereferencing NULL pointer 'Pair.Key'
//         {
//         }
//     }
// }
//
// This seems to be a combination of the following:
// - Dereferencing an iterator in a loop
// - Iterator type is not a pointer.
// - Key is a pointer type.
// - Key and Value are members of a base class.
// - Dereferencing is done as part of a compound boolean expression (removing '&& Pair.Value != 15' removes the warning)
#if (defined(_MSC_VER) && !defined(__clang__)) && USING_CODE_ANALYSIS
	#define UE_TUPLE_STATIC_ANALYSIS_WORKAROUND 1
#else
	#define UE_TUPLE_STATIC_ANALYSIS_WORKAROUND 0
#endif

#if defined(__cpp_structured_bindings)
	#define UE_TUPLE_STRUCTURED_BINDING_SUPPORT 1
#else
	#define UE_TUPLE_STRUCTURED_BINDING_SUPPORT 0
#endif

class FArchive;

template <typename... Types>
struct TTuple;

namespace UE::Core::Private::Tuple
{
	enum EForwardingConstructor { ForwardingConstructor };
	enum EOtherTupleConstructor { OtherTupleConstructor };

	// This only exists to have something to expand a parameter pack into, for concept checking
	template <typename... ArgTypes>
	void ConceptCheckingHelper(ArgTypes&&...);

	template <typename T, typename... Types>
	struct TTypeCountInParameterPack;

	template <typename T>
	struct TTypeCountInParameterPack<T>
	{
		constexpr static uint32 Value = 0;
	};

	template <typename T, typename U, typename... Types>
	struct TTypeCountInParameterPack<T, U, Types...>
	{
		constexpr static uint32 Value = TTypeCountInParameterPack<T, Types...>::Value + (std::is_same<T, U>::value ? 1 : 0);
	};

	template <typename T, uint32 Index, uint32 TupleSize>
	struct TTupleBaseElement
	{
		template <
			typename ArgType,
			typename TEnableIf<TIsConstructible<T, ArgType&&>::Value>::Type* = nullptr
		>
		explicit TTupleBaseElement(EForwardingConstructor, ArgType&& Arg)
			: Value(Forward<ArgType>(Arg))
		{
		}

		TTupleBaseElement()
			: Value()
		{
		}

		TTupleBaseElement(TTupleBaseElement&&) = default;
		TTupleBaseElement(const TTupleBaseElement&) = default;
		TTupleBaseElement& operator=(TTupleBaseElement&&) = default;
		TTupleBaseElement& operator=(const TTupleBaseElement&) = default;

		T Value;
	};

	template <typename T>
	struct TTupleBaseElement<T, 0, 2>
	{
		template <
			typename ArgType,
			typename TEnableIf<TIsConstructible<T, ArgType&&>::Value>::Type* = nullptr
		>
		explicit TTupleBaseElement(EForwardingConstructor, ArgType&& Arg)
			: Key(Forward<ArgType>(Arg))
		{
		}

		TTupleBaseElement()
			: Key()
		{
		}

		TTupleBaseElement(TTupleBaseElement&&) = default;
		TTupleBaseElement(const TTupleBaseElement&) = default;
		TTupleBaseElement& operator=(TTupleBaseElement&&) = default;
		TTupleBaseElement& operator=(const TTupleBaseElement&) = default;

		T Key;
	};

	template <uint32 Index, uint32 TupleSize>
	struct TTupleElementGetterByIndex
	{
		template <typename DeducedType, typename TupleType>
		static FORCEINLINE decltype(auto) GetImpl(const volatile TTupleBaseElement<DeducedType, Index, TupleSize>&, TupleType&& Tuple)
		{
			// Brackets are important here - we want a reference type to be returned, not object type
			decltype(auto) Result = (ForwardAsBase<TupleType, TTupleBaseElement<DeducedType, Index, TupleSize>>(Tuple).Value);

			// Keep tuple rvalue references to rvalue reference elements as rvalues, because that's how std::get() works, not how C++ struct member access works.
			return static_cast<std::conditional_t<TAnd<TNot<TIsReferenceType<TupleType>>, TIsRValueReferenceType<DeducedType>>::Value, DeducedType, decltype(Result)>>(Result);
		}

		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			return GetImpl(Tuple, Forward<TupleType>(Tuple));
		}
	};

#if UE_TUPLE_STATIC_ANALYSIS_WORKAROUND
	template <>
	struct TTupleElementGetterByIndex<0, 2>
	{
		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			// Brackets are important here - we want a reference type to be returned, not object type
			decltype(auto) Result = (Forward<TupleType>(Tuple).Key);

			// Keep tuple rvalue references to rvalue reference elements as rvalues, because that's how std::get() works, not how C++ struct member access works.
			return static_cast<std::conditional_t<TAnd<TNot<TIsReferenceType<TupleType>>, TIsRValueReferenceType<decltype(Tuple.Key)>>::Value, decltype(Tuple.Key), decltype(Result)>>(Result);
		}
	};
	template <>
	struct TTupleElementGetterByIndex<1, 2>
	{
		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			// Brackets are important here - we want a reference type to be returned, not object type
			decltype(auto) Result = (Forward<TupleType>(Tuple).Value);

			// Keep tuple rvalue references to rvalue reference elements as rvalues, because that's how std::get() works, not how C++ struct member access works.
			return static_cast<std::conditional_t<TAnd<TNot<TIsReferenceType<TupleType>>, TIsRValueReferenceType<decltype(Tuple.Value)>>::Value, decltype(Tuple.Value), decltype(Result)>>(Result);
		}
	};
#else
	template <>
	struct TTupleElementGetterByIndex<0, 2>
	{
		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			// Brackets are important here - we want a reference type to be returned, not object type
			decltype(auto) Result = (ForwardAsBase<TupleType, TTupleBaseElement<decltype(Tuple.Key), 0, 2>>(Tuple).Key);

			// Keep tuple rvalue references to rvalue reference elements as rvalues, because that's how std::get() works, not how C++ struct member access works.
			return static_cast<std::conditional_t<TAnd<TNot<TIsReferenceType<TupleType>>, TIsRValueReferenceType<decltype(Tuple.Key)>>::Value, decltype(Tuple.Key), decltype(Result)>>(Result);
		}
	};
#endif

	template <typename Type, uint32 TupleSize>
	struct TTupleElementGetterByType
	{
		template <uint32 DeducedIndex, typename TupleType>
		static FORCEINLINE decltype(auto) GetImpl(const volatile TTupleBaseElement<Type, DeducedIndex, TupleSize>&, TupleType&& Tuple)
		{
			return TTupleElementGetterByIndex<DeducedIndex, TupleSize>::Get(Forward<TupleType>(Tuple));
		}

		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			return GetImpl(Tuple, Forward<TupleType>(Tuple));
		}
	};

#if UE_TUPLE_STATIC_ANALYSIS_WORKAROUND
	template <typename Type>
	struct TTupleElementGetterByType<Type, 2>
	{
		template <typename TupleType>
		static FORCEINLINE decltype(auto) Get(TupleType&& Tuple)
		{
			if constexpr (std::is_same_v<Type, decltype(Tuple.Key)>)
			{
				return TTupleElementGetterByIndex<0, 2>::Get(Forward<TupleType>(Tuple));
			}
			else
			{
				static_assert(std::is_same_v<Type, decltype(Tuple.Value)>, "This has to be true - check the call site");
				return TTupleElementGetterByIndex<1, 2>::Get(Forward<TupleType>(Tuple));
			}
		}
	};
#endif

	template <uint32 ArgCount, uint32 ArgToCompare>
	struct FEqualityHelper
	{
		template <typename TupleType>
		FORCEINLINE static bool Compare(const TupleType& Lhs, const TupleType& Rhs)
		{
			return Lhs.template Get<ArgToCompare>() == Rhs.template Get<ArgToCompare>() && FEqualityHelper<ArgCount, ArgToCompare + 1>::Compare(Lhs, Rhs);
		}
	};

	template <uint32 ArgCount>
	struct FEqualityHelper<ArgCount, ArgCount>
	{
		template <typename TupleType>
		FORCEINLINE static bool Compare(const TupleType& Lhs, const TupleType& Rhs)
		{
			return true;
		}
	};

	template <uint32 NumArgs, uint32 ArgToCompare = 0, bool Last = ArgToCompare + 1 == NumArgs>
	struct TLessThanHelper
	{
		template <typename TupleType>
		FORCEINLINE static bool Do(const TupleType& Lhs, const TupleType& Rhs)
		{
			return Lhs.template Get<ArgToCompare>() < Rhs.template Get<ArgToCompare>() || (!(Rhs.template Get<ArgToCompare>() < Lhs.template Get<ArgToCompare>()) && TLessThanHelper<NumArgs, ArgToCompare + 1>::Do(Lhs, Rhs));
		}
	};

	template <uint32 NumArgs, uint32 ArgToCompare>
	struct TLessThanHelper<NumArgs, ArgToCompare, true>
	{
		template <typename TupleType>
		FORCEINLINE static bool Do(const TupleType& Lhs, const TupleType& Rhs)
		{
			return Lhs.template Get<ArgToCompare>() < Rhs.template Get<ArgToCompare>();
		}
	};

	template <uint32 NumArgs>
	struct TLessThanHelper<NumArgs, NumArgs, false>
	{
		template <typename TupleType>
		FORCEINLINE static bool Do(const TupleType& Lhs, const TupleType& Rhs)
		{
			return false;
		}
	};

	template <typename Indices, typename... Types>
	struct TTupleBase;

	template <uint32... Indices, typename... Types>
	struct TTupleBase<TIntegerSequence<uint32, Indices...>, Types...> : TTupleBaseElement<Types, Indices, sizeof...(Types)>...
	{
		template <
			typename... ArgTypes,
			decltype(ConceptCheckingHelper(TTupleBaseElement<Types, Indices, sizeof...(Types)>(ForwardingConstructor, DeclVal<ArgTypes&&>())...))* = nullptr
		>
		explicit TTupleBase(EForwardingConstructor, ArgTypes&&... Args)
			: TTupleBaseElement<Types, Indices, sizeof...(Types)>(ForwardingConstructor, Forward<ArgTypes>(Args))...
		{
		}

		template <
			typename TupleType,
			decltype(ConceptCheckingHelper(TTupleBaseElement<Types, Indices, sizeof...(Types)>(ForwardingConstructor, DeclVal<TupleType&&>().template Get<Indices>())...))* = nullptr
		>
		explicit TTupleBase(EOtherTupleConstructor, TupleType&& Other)
			: TTupleBaseElement<Types, Indices, sizeof...(Types)>(ForwardingConstructor, Forward<TupleType>(Other).template Get<Indices>())...
		{
		}

		TTupleBase() = default;
		TTupleBase(TTupleBase&& Other) = default;
		TTupleBase(const TTupleBase& Other) = default;
		TTupleBase& operator=(TTupleBase&& Other) = default;
		TTupleBase& operator=(const TTupleBase& Other) = default;

		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               &  { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<               TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         &  { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<const          TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&  { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<      volatile TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&  { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<const volatile TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               && { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<               TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         && { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<const          TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&& { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<      volatile TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < sizeof...(Types))>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&& { return TTupleElementGetterByIndex<Index, sizeof...(Types)>::Get(static_cast<const volatile TTupleBase&&>(*this)); }

		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               &  { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<               TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         &  { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<const          TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&  { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<      volatile TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&  { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<const volatile TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               && { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<               TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         && { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<const          TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&& { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<      volatile TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, Types...>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&& { return TTupleElementGetterByType<T, sizeof...(Types)>::Get(static_cast<const volatile TTupleBase&&>(*this)); }

		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)               &  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<               TTupleBase& >(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const         &  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const          TTupleBase& >(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)       volatile&  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<      volatile TTupleBase& >(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const volatile&  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const volatile TTupleBase& >(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)               && { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<               TTupleBase&&>(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const         && { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const          TTupleBase&&>(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)       volatile&& { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<      volatile TTupleBase&&>(*this).template Get<Indices>()...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const volatile&& { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const volatile TTupleBase&&>(*this).template Get<Indices>()...); }

		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)               &  { return ::Invoke(Func, static_cast<               TTupleBase& >(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const         &  { return ::Invoke(Func, static_cast<const          TTupleBase& >(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)       volatile&  { return ::Invoke(Func, static_cast<      volatile TTupleBase& >(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const volatile&  { return ::Invoke(Func, static_cast<const volatile TTupleBase& >(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)               && { return ::Invoke(Func, static_cast<               TTupleBase&&>(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const         && { return ::Invoke(Func, static_cast<const          TTupleBase&&>(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)       volatile&& { return ::Invoke(Func, static_cast<      volatile TTupleBase&&>(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const volatile&& { return ::Invoke(Func, static_cast<const volatile TTupleBase&&>(*this).template Get<Indices>()..., Forward<ArgTypes>(Args)...); }

		FORCEINLINE friend FArchive& operator<<(FArchive& Ar, TTupleBase& Tuple)
		{
			// This should be implemented with a fold expression when our compilers support it
			int Temp[] = { 0, (Ar << Tuple.template Get<Indices>(), 0)... };
			(void)Temp;
			return Ar;
		}

		FORCEINLINE friend void operator<<(FStructuredArchive::FSlot Slot, TTupleBase& Tuple)
		{
			// This should be implemented with a fold expression when our compilers support it
			FStructuredArchive::FStream Stream = Slot.EnterStream();
			int Temp[] = { 0, (Stream.EnterElement() << Tuple.template Get<Indices>(), 0)... };
			(void)Temp;
		}

		FORCEINLINE friend bool operator==(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			// This could be implemented with a fold expression when our compilers support it
			return FEqualityHelper<sizeof...(Types), 0>::Compare(Lhs, Rhs);
		}

		FORCEINLINE friend bool operator!=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Lhs == Rhs);
		}

		FORCEINLINE friend bool operator<(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return TLessThanHelper<sizeof...(Types)>::Do(Lhs, Rhs);
		}

		FORCEINLINE friend bool operator<=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Rhs < Lhs);
		}

		FORCEINLINE friend bool operator>(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return Rhs < Lhs;
		}

		FORCEINLINE friend bool operator>=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Lhs < Rhs);
		}
	};

#if UE_TUPLE_STATIC_ANALYSIS_WORKAROUND
	template <typename KeyType, typename ValueType>
	struct TTupleBase<TIntegerSequence<uint32, 0, 1>, KeyType, ValueType>
	{
		KeyType   Key;
		ValueType Value;

		using DummyPairIdentifier = void;

		template <
			typename KeyArgType,
			typename ValueArgType,
			typename TEnableIf<TIsConstructible<KeyType,   KeyArgType  &&>::Value>::Type* = nullptr,
			typename TEnableIf<TIsConstructible<ValueType, ValueArgType&&>::Value>::Type* = nullptr
		>
		explicit TTupleBase(EForwardingConstructor, KeyArgType&& KeyArg, ValueArgType&& ValueArg)
			: Key  (Forward<KeyArgType  >(KeyArg  ))
			, Value(Forward<ValueArgType>(ValueArg))
		{
		}

		template <
			typename TupleType,
			typename TupleType::DummyPairIdentifier* = nullptr,
			typename TEnableIf<TIsConstructible<KeyType,   decltype(DeclVal<TupleType&&>().Get<0>())>::Value>::Type* = nullptr,
			typename TEnableIf<TIsConstructible<ValueType, decltype(DeclVal<TupleType&&>().Get<1>())>::Value>::Type* = nullptr
		>
		explicit TTupleBase(EOtherTupleConstructor, TupleType&& Other)
			: Key  (Forward<TupleType>(Other).Get<0>())
			, Value(Forward<TupleType>(Other).Get<1>())
		{
		}

		TTupleBase() = default;
		TTupleBase(TTupleBase&& Other) = default;
		TTupleBase(const TTupleBase& Other) = default;
		TTupleBase& operator=(TTupleBase&& Other) = default;
		TTupleBase& operator=(const TTupleBase& Other) = default;

		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               &  { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<               TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         &  { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<const          TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&  { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<      volatile TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&  { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<const volatile TTupleBase& >(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               && { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<               TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         && { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<const          TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&& { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<      volatile TTupleBase&&>(*this)); }
		template <uint32 Index, typename TEnableIf<(Index < 2)>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&& { return TTupleElementGetterByIndex<Index, 2>::Get(static_cast<const volatile TTupleBase&&>(*this)); }

		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               &  { return TTupleElementGetterByType<T, 2>::Get(static_cast<               TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         &  { return TTupleElementGetterByType<T, 2>::Get(static_cast<const          TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&  { return TTupleElementGetterByType<T, 2>::Get(static_cast<      volatile TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&  { return TTupleElementGetterByType<T, 2>::Get(static_cast<const volatile TTupleBase& >(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()               && { return TTupleElementGetterByType<T, 2>::Get(static_cast<               TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const         && { return TTupleElementGetterByType<T, 2>::Get(static_cast<const          TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get()       volatile&& { return TTupleElementGetterByType<T, 2>::Get(static_cast<      volatile TTupleBase&&>(*this)); }
		template <typename T, typename TEnableIf<TTypeCountInParameterPack<T, KeyType, ValueType>::Value == 1>::Type* = nullptr> FORCEINLINE decltype(auto) Get() const volatile&& { return TTupleElementGetterByType<T, 2>::Get(static_cast<const volatile TTupleBase&&>(*this)); }

		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)               &  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<               TTupleBase& >(*this).template Get<0>(), static_cast<               TTupleBase& >(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const         &  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const          TTupleBase& >(*this).template Get<0>(), static_cast<const          TTupleBase& >(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)       volatile&  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<      volatile TTupleBase& >(*this).template Get<0>(), static_cast<      volatile TTupleBase& >(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const volatile&  { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const volatile TTupleBase& >(*this).template Get<0>(), static_cast<const volatile TTupleBase& >(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)               && { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<               TTupleBase&&>(*this).template Get<0>(), static_cast<               TTupleBase&&>(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const         && { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const          TTupleBase&&>(*this).template Get<0>(), static_cast<const          TTupleBase&&>(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args)       volatile&& { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<      volatile TTupleBase&&>(*this).template Get<0>(), static_cast<      volatile TTupleBase&&>(*this).template Get<1>()); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyAfter(FuncType&& Func, ArgTypes&&... Args) const volatile&& { return ::Invoke(Func, Forward<ArgTypes>(Args)..., static_cast<const volatile TTupleBase&&>(*this).template Get<0>(), static_cast<const volatile TTupleBase&&>(*this).template Get<1>()); }

		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)               &  { return ::Invoke(Func, static_cast<               TTupleBase& >(*this).template Get<0>(), static_cast<               TTupleBase& >(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const         &  { return ::Invoke(Func, static_cast<const          TTupleBase& >(*this).template Get<0>(), static_cast<const          TTupleBase& >(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)       volatile&  { return ::Invoke(Func, static_cast<      volatile TTupleBase& >(*this).template Get<0>(), static_cast<      volatile TTupleBase& >(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const volatile&  { return ::Invoke(Func, static_cast<const volatile TTupleBase& >(*this).template Get<0>(), static_cast<const volatile TTupleBase& >(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)               && { return ::Invoke(Func, static_cast<               TTupleBase&&>(*this).template Get<0>(), static_cast<               TTupleBase&&>(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const         && { return ::Invoke(Func, static_cast<const          TTupleBase&&>(*this).template Get<0>(), static_cast<const          TTupleBase&&>(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args)       volatile&& { return ::Invoke(Func, static_cast<      volatile TTupleBase&&>(*this).template Get<0>(), static_cast<      volatile TTupleBase&&>(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }
		template <typename FuncType, typename... ArgTypes> decltype(auto) ApplyBefore(FuncType&& Func, ArgTypes&&... Args) const volatile&& { return ::Invoke(Func, static_cast<const volatile TTupleBase&&>(*this).template Get<0>(), static_cast<const volatile TTupleBase&&>(*this).template Get<1>(), Forward<ArgTypes>(Args)...); }

		FORCEINLINE friend FArchive& operator<<(FArchive& Ar, TTupleBase& Tuple)
		{
			Ar << Tuple.Key;
			Ar << Tuple.Value;
			return Ar;
		}

		FORCEINLINE friend void operator<<(FStructuredArchive::FSlot Slot, TTupleBase& Tuple)
		{
			FStructuredArchive::FStream Stream = Slot.EnterStream();
			Stream.EnterElement() << Tuple.Key;
			Stream.EnterElement() << Tuple.Value;
		}

		FORCEINLINE friend bool operator==(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			// This could be implemented with a fold expression when our compilers support it
			return FEqualityHelper<2, 0>::Compare(Lhs, Rhs);
		}

		FORCEINLINE friend bool operator!=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Lhs == Rhs);
		}

		FORCEINLINE friend bool operator<(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return TLessThanHelper<2>::Do(Lhs, Rhs);
		}

		FORCEINLINE friend bool operator<=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Rhs < Lhs);
		}

		FORCEINLINE friend bool operator>(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return Rhs < Lhs;
		}

		FORCEINLINE friend bool operator>=(const TTupleBase& Lhs, const TTupleBase& Rhs)
		{
			return !(Lhs < Rhs);
		}
	};
#endif

	template <
		typename LhsType,
		typename RhsType,
		uint32... Indices,
		decltype(ConceptCheckingHelper((DeclVal<LhsType&>().template Get<Indices>() = DeclVal<RhsType&&>().template Get<Indices>(), 0)...))* = nullptr
	>
	static void Assign(LhsType& Lhs, RhsType&& Rhs, TIntegerSequence<uint32, Indices...>)
	{
		// This should be implemented with a fold expression when our compilers support it
		int Temp[] = { 0, (Lhs.template Get<Indices>() = Forward<RhsType>(Rhs).template Get<Indices>(), 0)... };
		(void)Temp;
	}

	template <typename... Types>
	FORCEINLINE TTuple<typename TDecay<Types>::Type...> MakeTupleImpl(Types&&... Args)
	{
		return TTuple<typename TDecay<Types>::Type...>(Forward<Types>(Args)...);
	}

	template <typename IntegerSequence>
	struct TTransformTuple_Impl;

	template <uint32... Indices>
	struct TTransformTuple_Impl<TIntegerSequence<uint32, Indices...>>
	{
		template <typename TupleType, typename FuncType>
		static decltype(auto) Do(TupleType&& Tuple, FuncType Func)
		{
			return MakeTupleImpl(Func(Forward<TupleType>(Tuple).template Get<Indices>())...);
		}
	};

	template <typename IntegerSequence>
	struct TVisitTupleElements_Impl;

	template <uint32... Indices>
	struct TVisitTupleElements_Impl<TIntegerSequence<uint32, Indices...>>
	{
		// We need a second function to do the invocation for a particular index, to avoid the pack expansion being
		// attempted on the indices and tuples simultaneously.
		template <uint32 Index, typename FuncType, typename... TupleTypes>
		FORCEINLINE static void InvokeFunc(FuncType&& Func, TupleTypes&&... Tuples)
		{
			::Invoke(Forward<FuncType>(Func), Forward<TupleTypes>(Tuples).template Get<Index>()...);
		}

		template <typename FuncType, typename... TupleTypes>
		static void Do(FuncType&& Func, TupleTypes&&... Tuples)
		{
			// This should be implemented with a fold expression when our compilers support it
			int Temp[] = { 0, (InvokeFunc<Indices>(Forward<FuncType>(Func), Forward<TupleTypes>(Tuples)...), 0)... };
			(void)Temp;
		}
	};


	template <typename TupleType>
	struct TCVTupleArity;

	template <typename... Types>
	struct TCVTupleArity<const volatile TTuple<Types...>>
	{
		enum { Value = sizeof...(Types) };
	};

	template <typename Type, typename TupleType>
	struct TCVTupleIndex
	{
		static_assert(sizeof(TupleType) == 0, "TTupleIndex instantiated with a non-tuple type");
		static constexpr uint32 Value = 0;
	};

	template <typename Type, typename... TupleTypes>
	struct TCVTupleIndex<Type, const volatile TTuple<TupleTypes...>>
	{
		static_assert(TTypeCountInParameterPack<Type, TupleTypes...>::Value >= 1, "TTupleIndex instantiated with a tuple which does not contain the type");
		static_assert(TTypeCountInParameterPack<Type, TupleTypes...>::Value <= 1, "TTupleIndex instantiated with a tuple which contains multiple instances of the type");

	private:
		template <uint32 DeducedIndex>
		static auto Resolve(TTupleBaseElement<Type, DeducedIndex, sizeof...(TupleTypes)>*) -> char(&)[DeducedIndex + 1];
		static auto Resolve(...) -> char;

	public:
		static constexpr uint32 Value = sizeof(Resolve(DeclVal<TTuple<TupleTypes...>*>())) - 1;
	};

#if UE_TUPLE_STATIC_ANALYSIS_WORKAROUND
	template <typename Type, typename KeyType, typename ValueType>
	struct TCVTupleIndex<Type, const volatile TTuple<KeyType, ValueType>>
	{
		static_assert(TTypeCountInParameterPack<Type, KeyType, ValueType>::Value >= 1, "TTupleIndex instantiated with a tuple which does not contain the type");
		static_assert(TTypeCountInParameterPack<Type, KeyType, ValueType>::Value <= 1, "TTupleIndex instantiated with a tuple which contains multiple instances of the type");

		static constexpr uint32 Value = std::is_same<Type, ValueType>::value ? 1 : 0;
	};
#endif

	template <uint32 Index, typename TupleType>
	struct TCVTupleElement
	{
		static_assert(sizeof(TupleType) == 0, "TTupleElement instantiated with a non-tuple type");
		using Type = void;
	};

	template <uint32 Index, typename... TupleTypes>
	struct TCVTupleElement<Index, const volatile TTuple<TupleTypes...>>
	{
		static_assert(Index < sizeof...(TupleTypes), "TTupleElement instantiated with an invalid index");

#ifdef __clang__
		using Type = __type_pack_element<Index, TupleTypes...>;
#else
	private:
		template <typename DeducedType>
		static DeducedType Resolve(TTupleBaseElement<DeducedType, Index, sizeof...(TupleTypes)>*);
		static void Resolve(...);

	public:
		using Type = decltype(Resolve(DeclVal<TTuple<TupleTypes...>*>()));
#endif
	};

#if UE_TUPLE_STATIC_ANALYSIS_WORKAROUND
	template <uint32 Index, typename KeyType, typename ValueType>
	struct TCVTupleElement<Index, const volatile TTuple<KeyType, ValueType>>
	{
		static_assert(Index < 2, "TTupleElement instantiated with an invalid index");

		using Type = std::conditional_t<Index == 0, KeyType, ValueType>;
	};
#endif

	template <uint32 ArgToCombine, uint32 ArgCount>
	struct TGetTupleHashHelper
	{
		template <typename TupleType>
		FORCEINLINE static uint32 Do(uint32 Hash, const TupleType& Tuple)
		{
			return TGetTupleHashHelper<ArgToCombine + 1, ArgCount>::Do(HashCombineFast(Hash, GetTypeHash(Tuple.template Get<ArgToCombine>())), Tuple);
		}
	};

	template <uint32 ArgIndex>
	struct TGetTupleHashHelper<ArgIndex, ArgIndex>
	{
		template <typename TupleType>
		FORCEINLINE static uint32 Do(uint32 Hash, const TupleType& Tuple)
		{
			return Hash;
		}
	};

	template <typename... Given, typename... Deduced>
	std::enable_if_t<std::conjunction_v<std::is_constructible<Given, Deduced&&>...>> ConstructibleConceptCheck(Deduced&&...);

	template <typename... Given, typename... Deduced>
	decltype(ConceptCheckingHelper((DeclVal<Given>() = DeclVal<Deduced&&>(), 0)...)) AssignableConceptCheck(Deduced&&...);
}

template <typename... Types>
struct TTuple : UE::Core::Private::Tuple::TTupleBase<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...>
{
private:
	typedef UE::Core::Private::Tuple::TTupleBase<TMakeIntegerSequence<uint32, sizeof...(Types)>, Types...> Super;

public:
#ifdef __cpp_conditional_explicit
	template <
		typename... ArgTypes,
		decltype(UE::Core::Private::Tuple::ConstructibleConceptCheck<Types...>(DeclVal<ArgTypes&&>()...))* = nullptr
	>
	explicit(!std::conjunction_v<std::is_convertible<ArgTypes&&, Types>...>) TTuple(ArgTypes&&... Args)
		: Super(UE::Core::Private::Tuple::ForwardingConstructor, Forward<ArgTypes>(Args)...)
	{
	}
#else
	template <
		typename... ArgTypes,
		decltype(UE::Core::Private::Tuple::ConstructibleConceptCheck<Types...>(DeclVal<ArgTypes&&>()...))* = nullptr,
		std::enable_if_t<std::conjunction_v<std::is_convertible<ArgTypes&&, Types>...>>* = nullptr
	>
	TTuple(ArgTypes&&... Args)
		: Super(UE::Core::Private::Tuple::ForwardingConstructor, Forward<ArgTypes>(Args)...)
	{
	}

	template <
		typename... ArgTypes,
		decltype(UE::Core::Private::Tuple::ConstructibleConceptCheck<Types...>(DeclVal<ArgTypes&&>()...))* = nullptr,
		std::enable_if_t<!std::conjunction_v<std::is_convertible<ArgTypes&&, Types>...>>* = nullptr
	>
	explicit TTuple(ArgTypes&&... Args)
		: Super(UE::Core::Private::Tuple::ForwardingConstructor, Forward<ArgTypes>(Args)...)
	{
	}
#endif

	template <
		typename... OtherTypes,
		decltype(UE::Core::Private::Tuple::ConstructibleConceptCheck<Types...>(DeclVal<OtherTypes&&>()...))* = nullptr
	>
	TTuple(TTuple<OtherTypes...>&& Other)
		: Super(UE::Core::Private::Tuple::OtherTupleConstructor, MoveTemp(Other))
	{
	}

	template <
		typename... OtherTypes,
		decltype(UE::Core::Private::Tuple::ConstructibleConceptCheck<Types...>(DeclVal<const OtherTypes&>()...))* = nullptr
	>
	TTuple(const TTuple<OtherTypes...>& Other)
		: Super(UE::Core::Private::Tuple::OtherTupleConstructor, Other)
	{
	}

	TTuple() = default;
	TTuple(TTuple&&) = default;
	TTuple(const TTuple&) = default;
	TTuple& operator=(TTuple&&) = default;
	TTuple& operator=(const TTuple&) = default;

	template <
		typename... OtherTypes,
		decltype(UE::Core::Private::Tuple::AssignableConceptCheck<Types&...>(DeclVal<const OtherTypes&>()...))* = nullptr
	>
	TTuple& operator=(const TTuple<OtherTypes...>& Other)
	{
		UE::Core::Private::Tuple::Assign(*this, Other, TMakeIntegerSequence<uint32, sizeof...(Types)>{});
		return *this;
	}

	template <
		typename... OtherTypes,
		decltype(UE::Core::Private::Tuple::AssignableConceptCheck<Types&...>(DeclVal<OtherTypes&&>()...))* = nullptr
	>
	TTuple& operator=(TTuple<OtherTypes...>&& Other)
	{
		UE::Core::Private::Tuple::Assign(*this, MoveTemp(Other), TMakeIntegerSequence<uint32, sizeof...(OtherTypes)>{});
		return *this;
	}

#if UE_TUPLE_STRUCTURED_BINDING_SUPPORT
	// TTuple support for structured binding - not intended to be called directly
	template <int N> friend decltype(auto) get(               TTuple&  val) { return static_cast<               TTuple& >(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(const          TTuple&  val) { return static_cast<const          TTuple& >(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(      volatile TTuple&  val) { return static_cast<      volatile TTuple& >(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(const volatile TTuple&  val) { return static_cast<const volatile TTuple& >(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(               TTuple&& val) { return static_cast<               TTuple&&>(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(const          TTuple&& val) { return static_cast<const          TTuple&&>(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(      volatile TTuple&& val) { return static_cast<      volatile TTuple&&>(val).template Get<N>(); }
	template <int N> friend decltype(auto) get(const volatile TTuple&& val) { return static_cast<const volatile TTuple&&>(val).template Get<N>(); }
#endif
};

template <typename... Types>
FORCEINLINE uint32 GetTypeHash(const TTuple<Types...>& Tuple)
{
	return UE::Core::Private::Tuple::TGetTupleHashHelper<1u, sizeof...(Types)>::Do(GetTypeHash(Tuple.template Get<0>()), Tuple);
}

FORCEINLINE uint32 GetTypeHash(const TTuple<>& Tuple)
{
	return 0;
}

namespace Freeze
{
	template<typename KeyType, typename ValueType>
	void IntrinsicWriteMemoryImage(FMemoryImageWriter& Writer, const TTuple<KeyType, ValueType>& Object, const FTypeLayoutDesc&)
	{
		Writer.WriteObject(Object.Key);
		Writer.WriteObject(Object.Value);
	}

	template<typename KeyType, typename ValueType>
	uint32 IntrinsicUnfrozenCopy(const FMemoryUnfreezeContent& Context, const TTuple<KeyType, ValueType>& Object, void* OutDst)
	{
		TTuple<KeyType, ValueType>* DstObject = (TTuple<KeyType, ValueType>*)OutDst;
		Context.UnfreezeObject(Object.Key, &DstObject->Key);
		Context.UnfreezeObject(Object.Value, &DstObject->Value);
		return sizeof(Object);
	}

	template<typename KeyType, typename ValueType>
	uint32 IntrinsicAppendHash(const TTuple<KeyType, ValueType>* DummyObject, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams, FSHA1& Hasher)
	{
		return Freeze::AppendHashPair(StaticGetTypeLayoutDesc<KeyType>(), StaticGetTypeLayoutDesc<ValueType>(), LayoutParams, Hasher);
	}

	template<typename KeyType, typename ValueType>
	uint32 IntrinsicGetTargetAlignment(const TTuple<KeyType, ValueType>* DummyObject, const FTypeLayoutDesc& TypeDesc, const FPlatformTypeLayoutParameters& LayoutParams)
	{
		const uint32 KeyAlignment = GetTargetAlignment(StaticGetTypeLayoutDesc<KeyType>(), LayoutParams);
		const uint32 ValueAlignment = GetTargetAlignment(StaticGetTypeLayoutDesc<ValueType>(), LayoutParams);
		return FMath::Min(FMath::Max(KeyAlignment, ValueAlignment), LayoutParams.MaxFieldAlignment);
	}
}

DECLARE_TEMPLATE_INTRINSIC_TYPE_LAYOUT((template <typename KeyType, typename ValueType>), (TTuple<KeyType, ValueType>));

/**
 * Traits class which calculates the number of elements in a tuple.
 */
template <typename TupleType>
struct TTupleArity : UE::Core::Private::Tuple::TCVTupleArity<const volatile TupleType>
{
};


/**
 * Traits class which gets the tuple index of a given type from a given TTuple.
 * If the type doesn't appear, or appears more than once, a compile error is generated.
 *
 * Given Type = char, and Tuple = TTuple<int, float, char>,
 * TTupleIndex<Type, Tuple>::Value will be 2.
 */
template <typename Type, typename TupleType>
using TTupleIndex = UE::Core::Private::Tuple::TCVTupleIndex<Type, const volatile TupleType>;


/**
 * Traits class which gets the element type of a TTuple with the given index.
 * If the index is out of range, a compile error is generated.
 *
 * Given Index = 1, and Tuple = TTuple<int, float, char>,
 * TTupleElement<Index, Tuple>::Type will be float.
 */
template <uint32 Index, typename TupleType>
using TTupleElement = UE::Core::Private::Tuple::TCVTupleElement<Index, const volatile TupleType>;


/**
 * Makes a TTuple from some arguments.  The type of the TTuple elements are the decayed versions of the arguments.
 *
 * @param  Args  The arguments used to construct the tuple.
 * @return A tuple containing a copy of the arguments.
 *
 * Example:
 *
 * void Func(const int32 A, FString&& B)
 * {
 *     // Equivalent to:
 *     // TTuple<int32, const TCHAR*, FString> MyTuple(A, TEXT("Hello"), MoveTemp(B));
 *     auto MyTuple = MakeTuple(A, TEXT("Hello"), MoveTemp(B));
 * }
 */
template <typename... Types>
FORCEINLINE TTuple<typename TDecay<Types>::Type...> MakeTuple(Types&&... Args)
{
	return UE::Core::Private::Tuple::MakeTupleImpl(Forward<Types>(Args)...);
}


/**
 * Creates a new TTuple by applying a functor to each of the elements.
 *
 * @param  Tuple  The tuple to apply the functor to.
 * @param  Func   The functor to apply.
 *
 * @return A new tuple of the transformed elements.
 *
 * Example:
 *
 * float        Overloaded(int32 Arg);
 * char         Overloaded(const TCHAR* Arg);
 * const TCHAR* Overloaded(const FString& Arg);
 *
 * void Func(const TTuple<int32, const TCHAR*, FString>& MyTuple)
 * {
 *     // Equivalent to:
 *     // TTuple<float, char, const TCHAR*> TransformedTuple(Overloaded(MyTuple.Get<0>()), Overloaded(MyTuple.Get<1>()), Overloaded(MyTuple.Get<2>())));
 *     auto TransformedTuple = TransformTuple(MyTuple, [](const auto& Arg) { return Overloaded(Arg); });
 * }
 */
template <typename FuncType, typename... Types>
FORCEINLINE decltype(auto) TransformTuple(TTuple<Types...>&& Tuple, FuncType Func)
{
	return UE::Core::Private::Tuple::TTransformTuple_Impl<TMakeIntegerSequence<uint32, sizeof...(Types)>>::Do(MoveTemp(Tuple), MoveTemp(Func));
}

template <typename FuncType, typename... Types>
FORCEINLINE decltype(auto) TransformTuple(const TTuple<Types...>& Tuple, FuncType Func)
{
	return UE::Core::Private::Tuple::TTransformTuple_Impl<TMakeIntegerSequence<uint32, sizeof...(Types)>>::Do(Tuple, MoveTemp(Func));
}


/**
 * Visits each element in the specified tuples in parallel and applies them as arguments to the functor.
 * All specified tuples must have the same number of elements.
 *
 * @param  Func    The functor to apply.
 * @param  Tuples  The tuples whose elements are to be applied to the functor.
 *
 * Example:
 *
 * void Func(const TTuple<int32, const TCHAR*, FString>& Tuple1, const TTuple<bool, float, FName>& Tuple2)
 * {
 *     // Equivalent to:
 *     // Functor(Tuple1.Get<0>(), Tuple2.Get<0>());
 *     // Functor(Tuple1.Get<1>(), Tuple2.Get<1>());
 *     // Functor(Tuple1.Get<2>(), Tuple2.Get<2>());
 *     VisitTupleElements(Functor, Tuple1, Tuple2);
 * }
 */
template <typename FuncType, typename FirstTupleType, typename... TupleTypes>
FORCEINLINE void VisitTupleElements(FuncType&& Func, FirstTupleType&& FirstTuple, TupleTypes&&... Tuples)
{
	UE::Core::Private::Tuple::TVisitTupleElements_Impl<TMakeIntegerSequence<uint32, TTupleArity<typename TDecay<FirstTupleType>::Type>::Value>>::Do(Forward<FuncType>(Func), Forward<FirstTupleType>(FirstTuple), Forward<TupleTypes>(Tuples)...);
}

/**
 * Tie function for structured unpacking of tuples into individual variables.
 *
 * Example:
 *
 * TTuple<FString, float, TArray<int32>> SomeFunction();
 *
 * FString Ret1;
 * float Ret2;
 * TArray<int32> Ret3;
 *
 * Tie(Ret1, Ret2, Ret3) = SomeFunction();
 *
 * // Now Ret1, Ret2 and Ret3 contain the unpacked return values.
 */
template <typename... Types>
FORCEINLINE TTuple<Types&...> Tie(Types&... Args)
{
	return TTuple<Types&...>(Args...);
}

#if UE_TUPLE_STRUCTURED_BINDING_SUPPORT
// TTuple support for structured bindings
template <typename... ArgTypes>
struct std::tuple_size<TTuple<ArgTypes...>>
	: std::integral_constant<std::size_t, sizeof...(ArgTypes)>
{
};
template <std::size_t N, typename... ArgTypes>
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 9000
// libc++ 8.0 and older incorrectly defined std::tuple_element as class, whereas the standard
// says it should be a struct.
class std::tuple_element<N, TTuple<ArgTypes...>>
#else
struct std::tuple_element<N, TTuple<ArgTypes...>>
#endif
{
public:
	using type = typename TTupleElement<N, TTuple<ArgTypes...>>::Type;
};
#endif
