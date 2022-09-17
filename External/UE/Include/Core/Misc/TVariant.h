// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TVariantMeta.h"
#include "Templates/EnableIf.h"
#include "Templates/IsConstructible.h"
#include "Templates/Decay.h"

/**
 * A special tag used to indicate that in-place construction of a variant should take place.
 */
template <typename T>
struct TInPlaceType {};

/**
 * A special tag that can be used as the first type in a TVariant parameter pack if none of the other types can be default-constructed.
 */
struct FEmptyVariantState {};

/**
 * A type-safe union based loosely on std::variant. This flavor of variant requires that all the types in the declaring template parameter pack be unique.
 * Attempting to use the value of a Get() when the underlying type is different leads to undefined behavior.
 */
template <typename T, typename... Ts>
class TVariant final : private UE::Core::Private::TVariantStorage<T, Ts...>
{
	static_assert(!UE::Core::Private::TTypePackContainsDuplicates<T, Ts...>::Value, "All the types used in TVariant should be unique");
	static_assert(!UE::Core::Private::TContainsReferenceType<T, Ts...>::Value, "TVariant cannot hold reference types");

public:
	/** Default initialize the TVariant to the first type in the parameter pack */
	TVariant()
	{
		static_assert(TIsConstructible<T>::Value, "To default-initialize a TVariant, the first type in the parameter pack must be default constructible. Use FEmptyVariantState as the first type if none of the other types can be listed first.");
		new(&UE::Core::Private::CastToStorage(*this).Storage) T();
		TypeIndex = 0;
	}

	/** Perform in-place construction of a type into the variant */
	template <typename U, typename... TArgs>
	explicit TVariant(TInPlaceType<U>&&, TArgs&&... Args)
	{
		constexpr SIZE_T Index = UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TVariant is not declared to hold the type being constructed");

		new(&UE::Core::Private::CastToStorage(*this).Storage) U(Forward<TArgs>(Args)...);
		TypeIndex = Index;
	}

	/** Copy construct the variant from another variant of the same type */
	TVariant(const TVariant& Other)
		: TypeIndex(Other.TypeIndex)
	{
		UE::Core::Private::TCopyConstructorLookup<T, Ts...>::Construct(TypeIndex, &UE::Core::Private::CastToStorage(*this).Storage, &UE::Core::Private::CastToStorage(Other).Storage);
	}

	/** Move construct the variant from another variant of the same type */
	TVariant(TVariant&& Other)
		: TypeIndex(Other.TypeIndex)
	{
		UE::Core::Private::TMoveConstructorLookup<T, Ts...>::Construct(TypeIndex, &UE::Core::Private::CastToStorage(*this).Storage, &UE::Core::Private::CastToStorage(Other).Storage);
	}

	/** Copy assign a variant from another variant of the same type */
	TVariant& operator=(const TVariant& Other)
	{
		if (&Other != this)
		{
			TVariant Temp = Other;
			Swap(Temp, *this);
		}
		return *this;
	}

	/** Move assign a variant from another variant of the same type */
	TVariant& operator=(TVariant&& Other)
	{
		if (&Other != this)
		{
			TVariant Temp = MoveTemp(Other);
			Swap(Temp, *this);
		}
		return *this;
	}

	/** Destruct the underlying type (if appropriate) */
	~TVariant()
	{
		UE::Core::Private::TDestructorLookup<T, Ts...>::Destruct(TypeIndex, &UE::Core::Private::CastToStorage(*this).Storage);
	}

	/** Determine if the variant holds the specific type */
	template <typename U>
	bool IsType() const
	{
		static_assert(UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value != (SIZE_T)-1, "The TVariant is not declared to hold the type passed to IsType<>");
		return UE::Core::Private::TIsType<U, T, Ts...>::IsSame(TypeIndex);
	}

	/** Get a reference to the held value. Bad things can happen if this is called on a variant that does not hold the type asked for */
	template <typename U>
	U& Get()
	{
		constexpr SIZE_T Index = UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TVariant is not declared to hold the type passed to Get<>");

		check(Index == TypeIndex);
		// The intermediate step of casting to void* is used to avoid warnings due to use of reinterpret_cast between related types if U and the storage class are related
		// This was specifically encountered when U derives from TAlignedBytes
		return *reinterpret_cast<U*>(reinterpret_cast<void*>(&UE::Core::Private::CastToStorage(*this).Storage));
	}

	/** Get a reference to the held value. Bad things can happen if this is called on a variant that does not hold the type asked for */
	template <typename U>
	const U& Get() const
	{
		// Temporarily remove the const qualifier so we can implement Get in one location.
		return const_cast<TVariant*>(this)->template Get<U>();
	}

	/** Get a pointer to the held value if the held type is the same as the one specified */
	template <typename U>
	U* TryGet()
	{
		constexpr SIZE_T Index = UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TVariant is not declared to hold the type passed to TryGet<>");
		// The intermediate step of casting to void* is used to avoid warnings due to use of reinterpret_cast between related types if U and the storage class are related
		// This was specifically encountered when U derives from TAlignedBytes
		return Index == TypeIndex ? reinterpret_cast<U*>(reinterpret_cast<void*>(&UE::Core::Private::CastToStorage(*this).Storage)) : nullptr;
	}

	/** Get a pointer to the held value if the held type is the same as the one specified */
	template <typename U>
	const U* TryGet() const
	{
		// Temporarily remove the const qualifier so we can implement TryGet in one location.
		return const_cast<TVariant*>(this)->template TryGet<U>();
	}

	/** Set a specifically-typed value into the variant */
	template <typename U>
	void Set(typename TIdentity<U>::Type&& Value)
	{
		Emplace<U>(MoveTemp(Value));
	}

	/** Set a specifically-typed value into the variant */
	template <typename U>
	void Set(const typename TIdentity<U>::Type& Value)
	{
		Emplace<U>(Value);
	}

	/** Set a specifically-typed value into the variant using in-place construction */
	template <typename U, typename... TArgs>
	void Emplace(TArgs&&... Args)
	{
		constexpr SIZE_T Index = UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TVariant is not declared to hold the type passed to Emplace<>");

		UE::Core::Private::TDestructorLookup<T, Ts...>::Destruct(TypeIndex, &UE::Core::Private::CastToStorage(*this).Storage);
		new(&UE::Core::Private::CastToStorage(*this).Storage) U(Forward<TArgs>(Args)...);
		TypeIndex = Index;
	}

	/** Lookup the index of a type in the template parameter pack at compile time. */
	template <typename U>
	static constexpr SIZE_T IndexOfType()
	{
		constexpr SIZE_T Index = UE::Core::Private::TParameterPackTypeIndex<U, T, Ts...>::Value;
		static_assert(Index != (SIZE_T)-1, "The TVariant is not declared to hold the type passed to IndexOfType<>");
		return Index;
	}

	/** Returns the currently held type's index into the template parameter pack */
	SIZE_T GetIndex() const
	{
		return TypeIndex;
	}

private:
	/** Index into the template parameter pack for the type held. */
	SIZE_T TypeIndex;
};

/** Determine if a type is a variant */
template <typename T>
struct TIsVariant
{
	static constexpr bool Value = false;
};

template <typename... Ts>
struct TIsVariant<TVariant<Ts...>>
{
	static constexpr bool Value = true;
};

template <typename T> struct TIsVariant<T&> : public TIsVariant<T> {};
template <typename T> struct TIsVariant<T&&> : public TIsVariant<T> {};
template <typename T> struct TIsVariant<const T> : public TIsVariant<T> {};

/** Determine the number of types in a TVariant */
template <typename> struct TVariantSize;

template <typename... Ts>
struct TVariantSize<TVariant<Ts...>>
{
	static constexpr SIZE_T Value = sizeof...(Ts);
};

template <typename T> struct TVariantSize<T&> : public TVariantSize<T> {};
template <typename T> struct TVariantSize<T&&> : public TVariantSize<T> {};
template <typename T> struct TVariantSize<const T> : public TVariantSize<T> {};

/** Apply a visitor function to the list of variants */
template <
	typename Func,
	typename... Variants,
	typename = typename TEnableIf<UE::Core::Private::TIsAllVariant<typename TDecay<Variants>::Type...>::Value>::Type
>
decltype(auto) Visit(Func&& Callable, Variants&&... Args)
{
#if PLATFORM_COMPILER_HAS_FOLD_EXPRESSIONS
	constexpr SIZE_T NumPermutations = (1 * ... * (TVariantSize<Variants>::Value));
#else
	constexpr SIZE_T VariantSizes[] = { TVariantSize<Variants>::Value... };
	constexpr SIZE_T NumPermutations = UE::Core::Private::Multiply(VariantSizes, sizeof...(Variants));
#endif

	return UE::Core::Private::VisitImpl(
		UE::Core::Private::EncodeIndices(Args...),
		Forward<Func>(Callable),
		TMakeIntegerSequence<SIZE_T, NumPermutations>{},
		TMakeIntegerSequence<SIZE_T, sizeof...(Variants)>{},
		Forward<Variants>(Args)...
	);
}