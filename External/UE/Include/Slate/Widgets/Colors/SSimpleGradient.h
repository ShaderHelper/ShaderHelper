// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FPaintArgs;
class FSlateWindowElementList;

/**
 * Implements a Slate widget that renders a simple color gradient.
 */
class SLATE_API SSimpleGradient
	: public SCompoundWidget
{
public:

		SLATE_BEGIN_ARGS(SSimpleGradient)
			: _StartColor(FLinearColor(0.0f, 0.0f, 0.0f))
			, _EndColor(FLinearColor(1.0f, 1.0f, 1.0f))
			, _HasAlphaBackground(false)
			, _Orientation(Orient_Vertical)
		{ }

		/** The leftmost gradient color */
		SLATE_ATTRIBUTE(FLinearColor, StartColor)
		
		/** The rightmost gradient color */
		SLATE_ATTRIBUTE(FLinearColor, EndColor)

		/** Whether a checker background is displayed for alpha viewing */
		SLATE_ATTRIBUTE(bool, HasAlphaBackground)

		/** Horizontal or vertical gradient */
		SLATE_ATTRIBUTE(EOrientation, Orientation)

		/** Whether to display sRGB color */
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		UE_DEPRECATED(5.0, "UseSRGB is unused")
		SLATE_ARGUMENT_DEFAULT(bool, UseSRGB) = true;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

	SLATE_END_ARGS()

public:
	SSimpleGradient();

	/**
	 * Constructs the widget
	 *
	 * @param InArgs The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

protected:

	// SCompoundWidget overrides

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

private:

	/** The leftmost gradient color */
	TSlateAttribute<FLinearColor, EInvalidateWidgetReason::Paint> StartColor;

	/** The rightmost gradient color */
	TSlateAttribute<FLinearColor, EInvalidateWidgetReason::Paint> EndColor;

	/** Whether a checker background is displayed for alpha viewing */
	bool bHasAlphaBackground;
	
	/** Horizontal or vertical gradient */
	EOrientation Orientation;
};
