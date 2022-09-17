// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/UnrealTemplate.h"
#include "Misc/AssertionMacros.h"
#include "Containers/ArrayView.h"
#include "HAL/PlatformString.h" // for INT64_FMT

#include <type_traits>

/*
* TStridedView is similar to TArrayView, but allows flexible byte stride between elements.
* Stride must be positive and a multiple of the element alignment.
* Stride can be zero to duplicate the same element over the whole range.
* 
* Example usage:
* 
* struct FMyStruct
* {
*     uint32  SomeData;
*     FVector Position;
* };
*
* FVector ComputeMean(TStridedView<const FVector> Positions)
* {
*     return Algo::Accumulate(Positions, FVector(0.0f)) / Positions.Num();
* }
*
* FVector ComputeMeanPosition(TArrayView<const FMyStruct> Structs)
* {
*     return ComputeMean(MakeStridedView(Structs, &FMyStruct::Position))
* }
* 
* See StridedViewTest.cpp for more examples.
* 
*/

/** Pointer with extent and a stride, similar to TArrayView. Designed to allow functions to take pointers to arbitrarily structured data. */
template<typename InElementType, typename InSizeType = int32>
class TStridedView
{
public:

	using ElementType = InElementType;
	using SizeType = InSizeType;

	static_assert(std::is_signed_v<SizeType>, "TStridedView only supports signed index types");

	TStridedView() = default;

	template <typename OtherElementType, decltype(ImplicitConv<ElementType* const*>((OtherElementType**)nullptr))* = nullptr>
	FORCEINLINE TStridedView(SizeType InBytesBetweenElements, OtherElementType* InFirstElementPtr, SizeType InNumElements)
		: FirstElementPtr(InFirstElementPtr)
		, BytesBetweenElements(InBytesBetweenElements)
		, NumElements(InNumElements)
	{
		check(NumElements >= 0);
		check(BytesBetweenElements >= 0); // NOTE: Zero stride is valid to allow duplicating a single element.
		check(BytesBetweenElements % alignof(ElementType) == 0);
	}

	template <typename OtherElementType, decltype(ImplicitConv<ElementType* const*>((OtherElementType**)nullptr))* = nullptr>
	FORCEINLINE TStridedView(const TStridedView<OtherElementType, SizeType>& Other)
		: FirstElementPtr(nullptr)
		, BytesBetweenElements(Other.GetStride())
		, NumElements(Other.Num())
	{
		if (Other.Num())
		{
			FirstElementPtr = &Other[0];
		}
	}

	FORCEINLINE SizeType Num() const
	{
		return NumElements;
	}

	FORCEINLINE SizeType GetStride() const
	{
		return BytesBetweenElements;
	}

	FORCEINLINE ElementType& operator[](SizeType Index) const
	{
		return *GetElementPtr(Index);
	}

	struct FIterator
	{
		const TStridedView* Owner;
		SizeType Index;

		FORCEINLINE FIterator& operator++()
		{
			++Index;
			return *this;
		}

		FORCEINLINE ElementType& operator*()
		{
			return *Owner->GetElementPtrUnsafe(Index);
		}

		FORCEINLINE bool operator == (const FIterator& Other) const
		{
			return Owner == Other.Owner
				&& Index == Other.Index;
		}

		FORCEINLINE bool operator != (const FIterator& Other) const
		{
			return !(*this == Other);
		}
	};

	/** Ranged iteration support. DO NOT USE DIRECTLY. */

	FORCEINLINE FIterator begin() const { return FIterator{ this, 0 }; }
	FORCEINLINE FIterator end() const { return FIterator{ this, Num() }; }

private:

	FORCEINLINE void RangeCheck(SizeType Index) const
	{
		checkf((Index >= 0) & (Index < NumElements), TEXT("Array index out of bounds: %" INT64_FMT " from an array of size %" INT64_FMT), int64(Index), int64(NumElements))
	}

	FORCEINLINE ElementType* GetElementPtrUnsafe(SizeType Index) const
	{
		using ByteType = typename std::conditional_t<std::is_const_v<ElementType>, const uint8, uint8>;

		ByteType* AsBytes = reinterpret_cast<ByteType*>(FirstElementPtr);
		ElementType* AsElement = reinterpret_cast<ElementType*>(AsBytes + Index * BytesBetweenElements);

		return AsElement;
	}

	FORCEINLINE ElementType* GetElementPtr(SizeType Index) const
	{
		RangeCheck(Index);
		return GetElementPtrUnsafe(Index);
	}

private:

	ElementType* FirstElementPtr = nullptr;
	SizeType BytesBetweenElements = 0;
	SizeType NumElements = 0;
};

template <typename ElementType>
TStridedView<ElementType> MakeStridedView(int32 BytesBetweenElements, ElementType* FirstElement, int32 Count)
{
	return TStridedView<ElementType>(BytesBetweenElements, FirstElement, Count);
}

template <typename BaseStructureType, typename DerivedStructureType>
TStridedView<BaseStructureType> MakeStridedViewOfBase(TArrayView<DerivedStructureType> StructuredView)
{
	static_assert(TIsDerivedFrom<DerivedStructureType, BaseStructureType>::IsDerived, "Expecting derived structure type");
	return MakeStridedView<BaseStructureType>((int32)sizeof(DerivedStructureType), GetData(StructuredView), (int32)GetNum(StructuredView));
}

template <typename StructureType>
TStridedView<StructureType> MakeStridedView(TArrayView<StructureType> StructuredView)
{
	return MakeStridedView((int32)sizeof(StructureType), GetData(StructuredView), (int32)GetNum(StructuredView));
}

template <
	typename StructuredRangeType,
	typename ElementType,
	typename StructureType,
	decltype(GetData(std::declval<StructuredRangeType>()))* = nullptr,
	decltype(GetNum(std::declval<StructuredRangeType>()))* = nullptr
>
TStridedView<ElementType> MakeStridedView(StructuredRangeType&& StructuredRange, ElementType StructureType::* Member)
{
	return TStridedView<ElementType>((int32)sizeof(*GetData(StructuredRange)), &(GetData(StructuredRange)->*Member), (int32)GetNum(StructuredRange));
}

template <typename StructuredRangeType>
auto MakeStridedView(StructuredRangeType&& StructuredRange) -> TStridedView<std::remove_reference_t<decltype(*GetData(StructuredRange))>>
{
	auto StructuredView = MakeArrayView(Forward<StructuredRangeType>(StructuredRange));
	return MakeStridedView((int32)sizeof(*GetData(StructuredRange)), GetData(StructuredView), (int32)GetNum(StructuredView));
}

