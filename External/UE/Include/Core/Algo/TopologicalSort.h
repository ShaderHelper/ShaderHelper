// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Algo/KahnTopologicalSort.h"

namespace Algo
{
	/**
	 * Sorts the given range in leaf to root order.
	 *
	 * @param UniqueRange A range with element type T
	 *        Type T must support GetTypeHash and copy+move constructors; T being pointertype is recommended.
	 *        In/Out Variable, is sorted in place. Will be unmodified if function returns false.
	 * @param GetElementDependencies A callable with prototype that is one of
	 *            RangeType<T> GetElementDependencies(const T& Element)
	 *            const RangeType<T>& GetElementDependencies(const T& Element)
	 *        It should return the leaf-ward vertices of directed edges from the root-wards Element.
	 * @param Flags
	 *        ETopologicalSort::AllowCycles: If present, cycles will be allowed; an arbitrary vertex in the cycle
	 *            will be chosen first. If not present, the presence of a cycle will cause a failure to sort.
	 * 
	 * @return True if succeeded, false if failed due to cycles.
	 */
	template <typename RangeType, typename GetElementDependenciesType>
	FORCEINLINE bool TopologicalSort(RangeType& UniqueRange, GetElementDependenciesType GetElementDependencies,
		ETopologicalSort Flags = ETopologicalSort::None)
	{
		return KahnTopologicalSort(UniqueRange, Forward<GetElementDependenciesType>(GetElementDependencies), Flags);
	}
}