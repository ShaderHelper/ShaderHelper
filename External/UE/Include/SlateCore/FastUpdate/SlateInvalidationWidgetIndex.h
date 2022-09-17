// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SWidget;

/** Index of the special container to order widget in InvalidateRoot. See SlateInvalidationWidgetlist. */
struct FSlateInvalidationWidgetIndex
{
	friend class FSlateInvalidationWidgetList;
	friend struct FSlateInvalidationWidgetSortOrder;
	
public:
	static const FSlateInvalidationWidgetIndex Invalid;
	bool operator== (FSlateInvalidationWidgetIndex Other) const { return ArrayIndex == Other.ArrayIndex && ElementIndex == Other.ElementIndex; }
	bool operator!= (FSlateInvalidationWidgetIndex Other) const { return ArrayIndex != Other.ArrayIndex || ElementIndex != Other.ElementIndex; }
	FString ToString() const { return FString::Printf(TEXT("{%d, %d}"), ArrayIndex, ElementIndex); }
	
private:
	using IndexType = uint16;

	FSlateInvalidationWidgetIndex(IndexType InArray, IndexType InElement) : ArrayIndex(InArray), ElementIndex(InElement) {}

	/** Index of the array node in FSlateInvalidationWidgetList */
	IndexType ArrayIndex;
	/** Index of the element node in FSlateInvalidationWidgetList */
	IndexType ElementIndex;
};
