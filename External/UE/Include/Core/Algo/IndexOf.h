// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/IdentityFunctor.h"
#include "Templates/Invoke.h"
#include "Templates/UnrealTemplate.h"

namespace AlgoImpl
{
	template <typename RangeType, typename ValueType, typename ProjectionType>
	auto IndexOfBy(RangeType&& Range, const ValueType& Value, ProjectionType Proj)
	{
		auto Num = GetNum(Range);
		auto Data = GetData(Range);

		using SizeType = decltype(Num);
		for (SizeType Index = 0; Index < Num; ++Index)
		{
			if (Invoke(Proj, Data[Index]) == Value)
			{
				return Index;
			}
		}

		return (SizeType)-1;
	}

	template <typename RangeType, typename PredicateType>
	auto IndexOfByPredicate(RangeType&& Range, PredicateType Pred)
	{
		auto Num = GetNum(Range);
		auto Data = GetData(Range);

		using SizeType = decltype(Num);
		for (SizeType Index = 0; Index < Num; ++Index)
		{
			if (Invoke(Pred, Data[Index]))
			{
				return Index;
			}
		}

		return (SizeType)-1;
	}
}

namespace Algo
{
	/**
	 * Returns the index of the first element in the range which is equal to the given value.
	 *
	 * @param  Range  The range to search.
	 * @param  Value  The value to search for.
	 *
	 * @return The index of the first element found, or (SizeType)-1 if none was found (where SizeType = decltype(GetNum(Range)))
	 */
	template <typename RangeType, typename ValueType>
	FORCEINLINE auto IndexOf(RangeType&& Range, const ValueType& Value)
	{
		static_assert(TIsContiguousContainer<RangeType>::Value, "Must only be used with contiguous containers");
		return AlgoImpl::IndexOfBy(Forward<RangeType>(Range), Value, FIdentityFunctor());
	}

	/**
	 * Returns the index of the first element in the range whose projection is equal to the given value.
	 *
	 * @param  Range  The range to search.
	 * @param  Value  The value to search for.
	 * @param  Proj   The projection to apply to the element.
	 *
	 * @return The index of the first element found, or (SizeType)-1 if none was found (where SizeType = decltype(GetNum(Range)))
	 */
	template <typename RangeType, typename ValueType, typename ProjectionType>
	FORCEINLINE auto IndexOfBy(RangeType&& Range, const ValueType& Value, ProjectionType Proj)
	{
		static_assert(TIsContiguousContainer<RangeType>::Value, "Must only be used with contiguous containers");
		return AlgoImpl::IndexOfBy(Forward<RangeType>(Range), Value, MoveTemp(Proj));
	}

	/**
	 * Returns the index of the first element in the range which matches the predicate.
	 *
	 * @param  Range  The range to search.
	 * @param  Pred   The predicate to search for.
	 *
	 * @return The index of the first element found, or (SizeType)-1 if none was found (where SizeType = decltype(GetNum(Range)))
	 */
	template <typename RangeType, typename PredicateType>
	FORCEINLINE auto IndexOfByPredicate(RangeType&& Range, PredicateType Pred)
	{
		static_assert(TIsContiguousContainer<RangeType>::Value, "Must only be used with contiguous containers");
		return AlgoImpl::IndexOfByPredicate(Forward<RangeType>(Range), MoveTemp(Pred));
	}
}
