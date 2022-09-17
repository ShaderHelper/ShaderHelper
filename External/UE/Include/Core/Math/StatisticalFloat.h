// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Math/UnrealMathUtility.h"

// Used to measure a distribution
template <typename T>
struct FStatisticalValue
{
public:
	FStatisticalValue()
		: MinValue()
		, MaxValue()
		, Accumulator()
		, NumSamples()
	{
	}

	void AddSample(T Value)
	{
		if (NumSamples == 0)
		{
			MinValue = MaxValue = Value;
		}
		else
		{
			MinValue = FMath::Min(MinValue, Value);
			MaxValue = FMath::Max(MaxValue, Value);
		}
		Accumulator += Value;
		++NumSamples;
	}

	T GetMinValue() const
	{
		return MinValue;
	}

	T GetMaxValue() const
	{
		return MaxValue;
	}

	T GetAvgValue() const
	{
		return (NumSamples > 0) ? (Accumulator / (double)NumSamples) : T();
	}

	int32 GetCount() const
	{
		return NumSamples;
	}

private:
	T MinValue;
	T MaxValue;
	T Accumulator;
	int32 NumSamples;
};

typedef FStatisticalValue<double> FStatisticalFloat;