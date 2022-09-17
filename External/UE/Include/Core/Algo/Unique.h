// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/UnrealTemplate.h"
#include "Templates/Invoke.h"
#include "Templates/EqualTo.h"

namespace AlgoImpl
{
	template <typename T, typename SizeType, typename BinaryPredicate>
	SizeType Unique(T* Array, SizeType ArraySize, BinaryPredicate Predicate)
	{
		if (ArraySize <= 1)
		{
			return ArraySize;
		}

		T* Result = Array;
		for (T* Iter = Array + 1; Iter != Array + ArraySize; ++Iter)
		{
			if (!Invoke(Predicate, *Result, *Iter)) 
			{
				++Result;
				if (Result != Iter)
				{
					*Result = MoveTemp(*Iter);
				}
			}
		}

		return static_cast<SizeType>(Result + 1 - Array);
	}
}

namespace Algo
{
	/**
	 * Eliminates all but the first element from every consecutive group of equivalent elements and
	 * returns past-the-end index of unique elements for the new logical end of the range.
	 *
	 * Removing is done by shifting the elements in the range in such a way that elements to be erased are overwritten.
	 * Relative order of the elements that remain is preserved and the physical size of the range is unchanged.
	 * References to an element between the new logical end and the physical end of the range are still
	 * dereferenceable, but the elements themselves have unspecified values. A call to `Unique` is typically followed by a call
	 * to a container's `SetNum` method as:
	 *
	 * ```Container.SetNum(Algo::Unique(Container));```
	 *
	 * that erases the unspecified values and reduces the physical size of the container
	 * to match its new logical size.
	 *
	 * Elements are compared using operator== or given binary predicate. The behavior is undefined if it is not an equivalence relation.
	 *
	 * See https://en.cppreference.com/w/cpp/algorithm/unique
	 */
	template<typename RangeType>
	auto Unique(RangeType&& Range) -> decltype(AlgoImpl::Unique(GetData(Range), GetNum(Range), TEqualTo<>{}))
	{
		return AlgoImpl::Unique(GetData(Range), GetNum(Range), TEqualTo<>{});
	}

	template<typename RangeType, typename BinaryPredicate>
	auto Unique(RangeType&& Range, BinaryPredicate Predicate) -> decltype(AlgoImpl::Unique(GetData(Range), GetNum(Range), MoveTemp(Predicate)))
	{
		return AlgoImpl::Unique(GetData(Range), GetNum(Range), MoveTemp(Predicate));
	}
}
