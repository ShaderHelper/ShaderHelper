// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Math/UnrealMathUtility.h"
#include "Templates/UnrealTemplate.h"

namespace Algo
{
	/**
	 * Randomly shuffle a range of elements.
	 *
	 * @param  Range  Any contiguous container.
	 */
	template <typename RangeType>
	void RandomShuffle(RangeType& Range)
	{
		auto Data = GetData(Range);

		using SizeType = decltype(GetNum(Range));
		const SizeType Num = GetNum(Range);

		for (SizeType Index = 0; Index < Num - 1; ++Index)
		{
			// Get a random integer in [Index, Num)
			const SizeType RandomIndex = Index + FMath::RandHelper64(Num - Index);
			if (RandomIndex != Index)
			{
				Swap(Data[Index], Data[RandomIndex]);
			}
		}
	}
}
