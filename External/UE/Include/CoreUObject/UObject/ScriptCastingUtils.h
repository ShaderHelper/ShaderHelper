// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Stack.h"

template <typename, typename> struct TAreValidFloatingPointPairs : public std::false_type {};
template <> struct TAreValidFloatingPointPairs<float, double> : public std::true_type {};
template <> struct TAreValidFloatingPointPairs<double, float> : public std::true_type {};

template <typename, typename> struct TAreValidVectorPairs : public std::false_type {};
template <> struct TAreValidVectorPairs<FVector, FVector3f> : public std::true_type {};
template <> struct TAreValidVectorPairs<FVector3f, FVector> : public std::true_type {};

template <typename T>
constexpr bool TAlwaysFalse()
{
	return false;
}

using CastFunction = void (*)(const FProperty*, const void*, void*);

FORCEINLINE void ImplicitCast(float Source, double& Destination)
{
	Destination = static_cast<double>(Source);
}

FORCEINLINE void ImplicitCast(double Source, float& Destination)
{
	Destination = static_cast<float>(Source);
}

FORCEINLINE void ImplicitCast(const FVector3f& Source, FVector& Destination)
{
	Destination.X = static_cast<double>(Source.X);
	Destination.Y = static_cast<double>(Source.Y);
	Destination.Z = static_cast<double>(Source.Z);
}

FORCEINLINE void ImplicitCast(const FVector& Source, FVector3f& Destination)
{
	Destination.X = static_cast<float>(Source.X);
	Destination.Y = static_cast<float>(Source.Y);
	Destination.Z = static_cast<float>(Source.Z);
}

template <typename SourceType, typename DestinationType>
void FloatingPointCast(const FProperty* /* unused */, const void* SourceRawData, void* DestinationRawData)
{
	checkSlow(SourceRawData);
	checkSlow(DestinationRawData);

	const SourceType* Source = reinterpret_cast<const SourceType*>(SourceRawData);
	DestinationType* Destination = reinterpret_cast<DestinationType*>(DestinationRawData);

	if constexpr (TAreValidFloatingPointPairs<SourceType, DestinationType>::value || TAreValidVectorPairs<SourceType, DestinationType>::value)
	{
		ImplicitCast(*Source, *Destination);
	}
	else
	{
		static_assert(TAlwaysFalse<SourceType>(), "Unsupported type pairs used for casting!");
	}
}

FORCEINLINE void FloatToDoubleCast(const FProperty* /* unused */, const void* SourceRawData, void* DestinationRawData)
{
	FloatingPointCast<float, double>(nullptr, SourceRawData, DestinationRawData);
}

FORCEINLINE void DoubleToFloatCast(const FProperty* /* unused */, const void* SourceRawData, void* DestinationRawData)
{
	FloatingPointCast<double, float>(nullptr, SourceRawData, DestinationRawData);
}

FORCEINLINE void CopyElement(const FProperty* SourceProperty, const void* SourceRawData, void* DestinationRawData)
{
	checkSlow(SourceProperty);

	SourceProperty->CopySingleValue(DestinationRawData, SourceRawData);
}

template <typename SourceType, typename DestinationType>
void CopyAndCastArray(const FArrayProperty* SourceArrayProperty,
					  const void* SourceAddress,
					  const FArrayProperty* DestinationArrayProperty,
					  void* DestinationAddress)
{
	checkSlow(SourceArrayProperty);
	checkSlow(SourceAddress);
	checkSlow(DestinationArrayProperty);
	checkSlow(DestinationAddress);

	FScriptArrayHelper SourceArrayHelper(SourceArrayProperty, SourceAddress);
	FScriptArrayHelper DestinationArrayHelper(DestinationArrayProperty, DestinationAddress);

	DestinationArrayHelper.Resize(SourceArrayHelper.Num());
	for (int32 i = 0; i < SourceArrayHelper.Num(); ++i)
	{
		const void* SourceRawData = SourceArrayHelper.GetRawPtr(i);
		void* DestinationRawData = DestinationArrayHelper.GetRawPtr(i);

		FloatingPointCast<SourceType, DestinationType>(nullptr, SourceRawData, DestinationRawData);
	}
}

template <typename SourceType, typename DestinationType>
void CopyAndCastArrayFromStack(FFrame& Stack, RESULT_DECL)
{
	FArrayProperty* DestinationArrayProperty = ExactCastField<FArrayProperty>(Stack.MostRecentProperty);
	void* DestinationAddress = RESULT_PARAM;

	if (Stack.StepAndCheckMostRecentProperty(Stack.Object, nullptr))
	{
		FArrayProperty* SourceArrayProperty = ExactCastField<FArrayProperty>(Stack.MostRecentProperty);
		void* SourceAddress = Stack.MostRecentPropertyAddress;

		CopyAndCastArray<SourceType, DestinationType>(SourceArrayProperty, SourceAddress, DestinationArrayProperty, DestinationAddress);
	}
	else
	{
		UE_LOG(LogScript, Verbose, TEXT("Cast failed: recent properties were null!"));
	}
}

template <typename SourceType, typename DestinationType>
void CopyAndCastSetFromStack(FFrame& Stack, RESULT_DECL)
{
	FSetProperty* DestinationSetProperty = ExactCastField<FSetProperty>(Stack.MostRecentProperty);
	checkSlow(DestinationSetProperty);
	checkSlow(RESULT_PARAM);
	FScriptSetHelper DestinationSetHelper(DestinationSetProperty, RESULT_PARAM);

	if (Stack.StepAndCheckMostRecentProperty(Stack.Object, nullptr))
	{
		checkSlow(Stack.MostRecentProperty);
		checkSlow(Stack.MostRecentPropertyAddress);

		FSetProperty* SourceSetProperty = ExactCastField<FSetProperty>(Stack.MostRecentProperty);
		FScriptSetHelper SourceSetHelper(SourceSetProperty, Stack.MostRecentPropertyAddress);

		DestinationSetHelper.EmptyElements(SourceSetHelper.Num());
		for (int32 i = 0; i < SourceSetHelper.Num(); ++i)
		{
			int32 NewIndex = DestinationSetHelper.AddDefaultValue_Invalid_NeedsRehash();
			void* DestinationRawData = DestinationSetHelper.GetElementPtr(NewIndex);
			const void* SourceRawData = SourceSetHelper.GetElementPtr(i);

			FloatingPointCast<SourceType, DestinationType>(nullptr, SourceRawData, DestinationRawData);
		}
		DestinationSetHelper.Rehash();
	}
	else
	{
		UE_LOG(LogScript, Verbose, TEXT("Cast failed: recent properties were null!"));
	}
}

template <CastFunction KeyCastFunction, CastFunction ValueCastFunction>
void CopyAndCastMapFromStack(FFrame& Stack, RESULT_DECL)
{
	FMapProperty* DestinationMapProperty = ExactCastField<FMapProperty>(Stack.MostRecentProperty);
	checkSlow(DestinationMapProperty);

	FScriptMapHelper DestinationMapHelper(DestinationMapProperty, RESULT_PARAM);

	if (Stack.StepAndCheckMostRecentProperty(Stack.Object, nullptr))
	{
		FMapProperty* SourceMapProperty = ExactCastField<FMapProperty>(Stack.MostRecentProperty);
		checkSlow(SourceMapProperty);

		FScriptMapHelper SourceMapHelper(SourceMapProperty, Stack.MostRecentPropertyAddress);

		const FProperty* SourceKeyProperty = SourceMapProperty->KeyProp;
		checkSlow(SourceKeyProperty);

		const FProperty* SourceValueProperty = SourceMapProperty->ValueProp;
		checkSlow(SourceValueProperty);

		DestinationMapHelper.EmptyValues(SourceMapHelper.Num());
		for (int32 i = 0; i < SourceMapHelper.Num(); ++i)
		{
			int32 NewIndex = DestinationMapHelper.AddDefaultValue_Invalid_NeedsRehash();

			const void* SourceKeyRawData = SourceMapHelper.GetKeyPtr(i);
			void* DestinationKeyRawData = DestinationMapHelper.GetKeyPtr(NewIndex);

			KeyCastFunction(SourceKeyProperty, SourceKeyRawData, DestinationKeyRawData);

			const void* SourceValueRawData = SourceMapHelper.GetValuePtr(i);
			void* DestinationValueRawData = DestinationMapHelper.GetValuePtr(NewIndex);

			ValueCastFunction(SourceValueProperty, SourceValueRawData, DestinationValueRawData);
		}
		DestinationMapHelper.Rehash();
	}
	else
	{
		UE_LOG(LogScript, Verbose, TEXT("Cast failed: recent properties were null!"));
	}
}
