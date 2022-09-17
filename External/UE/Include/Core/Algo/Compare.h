// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Templates/EqualTo.h"
#include "Templates/IdentityFunctor.h"
#include "Templates/Invoke.h"
#include "Templates/UnrealTemplate.h"

namespace Algo::Private
{
	template <typename InAT, typename InBT, typename ProjectionT, typename PredicateT>
	constexpr bool Compare(InAT&& InputA, InBT&& InputB, ProjectionT Projection, PredicateT Predicate)
	{
		const SIZE_T SizeA = GetNum(InputA);
		const SIZE_T SizeB = GetNum(InputB);

		if (SizeA != SizeB)
		{
			return false;
		}

		auto* A = GetData(InputA);
		auto* B = GetData(InputB);

		for (SIZE_T Count = SizeA; Count; --Count)
		{
			if (!Invoke(Predicate, Invoke(Projection, *A++), Invoke(Projection, *B++)))
			{
				return false;
			}
		}

		return true;
	}
}

namespace Algo
{
	/**
	 * Compares two contiguous containers using operator== to compare pairs of elements.
	 *
	 * @param  InputA     Container of elements that are used as the first argument to operator==.
	 * @param  InputB     Container of elements that are used as the second argument to operator==.
	 *
	 * @return Whether the containers are the same size and operator== returned true for every pair of elements.
	 */
	template <typename InAT, typename InBT>
	constexpr bool Compare(InAT&& InputA, InBT&& InputB)
	{
		return Private::Compare(Forward<InAT>(InputA), Forward<InBT>(InputB), FIdentityFunctor(), TEqualTo<>());
	}

	/**
	 * Compares two contiguous containers using a predicate to compare pairs of elements.
	 *
	 * @param  InputA     Container of elements that are used as the first argument to the predicate.
	 * @param  InputB     Container of elements that are used as the second argument to the predicate.
	 * @param  Predicate  Condition which returns true for elements which are deemed equal.
	 *
	 * @return Whether the containers are the same size and the predicate returned true for every pair of elements.
	 */
	template <typename InAT, typename InBT, typename PredicateT>
	constexpr bool Compare(InAT&& InputA, InBT&& InputB, PredicateT Predicate)
	{
		return Private::Compare(Forward<InAT>(InputA), Forward<InBT>(InputB), FIdentityFunctor(), MoveTemp(Predicate));
	}

	template <typename InAT, typename InBT, typename PredicateT>
	UE_DEPRECATED(5.0, "CompareByPredicate has been renamed to Compare.")
	constexpr bool CompareByPredicate(InAT&& InputA, InBT&& InputB, PredicateT Predicate)
	{
		return Compare(Forward<InAT>(InputA), Forward<InBT>(InputB), MoveTemp(Predicate));
	}

	/**
	 * Compares two contiguous containers using operator== to compare pairs of projected elements.
	 *
	 * @param  InputA     Container of elements that are used as the first argument to operator==.
	 * @param  InputB     Container of elements that are used as the second argument to operator==.
	 * @param  Projection Projection to apply to the elements before comparing them.
	 *
	 * @return Whether the containers are the same size and operator== returned true for every pair of elements.
	 */
	template <typename InAT, typename InBT, typename ProjectionT>
	constexpr bool CompareBy(InAT&& InputA, InBT&& InputB, ProjectionT Projection)
	{
		return Private::Compare(Forward<InAT>(InputA), Forward<InBT>(InputB), MoveTemp(Projection), TEqualTo<>());
	}

	/**
	 * Compares two contiguous containers using a predicate to compare pairs of projected elements.
	 *
	 * @param  InputA     Container of elements that are used as the first argument to the predicate.
	 * @param  InputB     Container of elements that are used as the second argument to the predicate.
	 * @param  Projection Projection to apply to the elements before comparing them.
	 * @param  Predicate  Condition which returns true for elements which are deemed equal.
	 *
	 * @return Whether the containers are the same size and the predicate returned true for every pair of elements.
	 */
	template <typename InAT, typename InBT, typename ProjectionT, typename PredicateT>
	constexpr bool CompareBy(InAT&& InputA, InBT&& InputB, ProjectionT Projection, PredicateT Predicate)
	{
		return Private::Compare(Forward<InAT>(InputA), Forward<InBT>(InputB), MoveTemp(Projection), MoveTemp(Predicate));
	}
}
