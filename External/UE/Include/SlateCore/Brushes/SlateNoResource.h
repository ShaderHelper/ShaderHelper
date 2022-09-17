// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"

/**
 * A resource that has no appearance
 */
struct SLATECORE_API FSlateNoResource
	: public FSlateBrush
{
	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InImageSize An optional image size (default is zero).
	 */
	FORCENOINLINE FSlateNoResource( const FVector2D& InImageSize = FVector2D::ZeroVector )
		: FSlateBrush(ESlateBrushDrawType::NoDrawType, FName(NAME_None), FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, InImageSize)
	{ }
};

/** 
 * This represents an undefined brush. FSlateNoResource is a valid brush which means don't render anything.  FSlateOptionalBrush means let the widget decide what to do if the brush is unset. 
 * Widgets that don't check for the brush being unspecified will draw nothing
 */
struct SLATECORE_API FSlateOptionalBrush
	: public FSlateBrush
{
	FSlateOptionalBrush(const FVector2D& InImageSize = FVector2D::ZeroVector)
		: FSlateBrush(ESlateBrushDrawType::NoDrawType, FName(NAME_None), FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::NoImage, InImageSize)
	{
		bIsSet = false;
	}
};