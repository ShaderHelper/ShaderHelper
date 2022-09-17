// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

/**
 * Temporary compatibility mechanism to be used solely for the purpose of raw pointers to wrapped pointers.
 * Specialization and use of this type is not supported beyond the specific instances in ObjectPtr.h.
 * These types and the container methods that leverage them will be removed in a future release.
 */
template <typename InElementType>
struct TContainerElementTypeCompatibility
{
	typedef InElementType ReinterpretType;

	template <typename IterBeginType, typename IterEndType, typename OperatorType = InElementType&(*)(IterBeginType&)>
	static void ReinterpretRange(IterBeginType Iter, IterEndType IterEnd, OperatorType Operator = [](IterBeginType& InIt) -> InElementType& { return *InIt; })
	{
	}

	typedef InElementType CopyFromOtherType;

	static constexpr void CopyingFromOtherType() {}
};

/**
 * Temporary compatibility mechanism to be used solely for the purpose of raw pointers to wrapped pointers.
 * These types and the container methods that leverage them will be removed in a future release.
 */
template <typename ElementType>
struct TIsContainerElementTypeReinterpretable
{
	enum { Value = !TIsSame<typename TContainerElementTypeCompatibility<ElementType>::ReinterpretType, ElementType>::Value };
};

/**
 * Temporary compatibility mechanism to be used solely for the purpose of raw pointers to wrapped pointers.
 * These types and the container methods that leverage them will be removed in a future release.
 */
template <typename ElementType>
struct TIsContainerElementTypeCopyable
{
	enum { Value = !TIsSame<typename TContainerElementTypeCompatibility<ElementType>::CopyFromOtherType, ElementType>::Value };
};
