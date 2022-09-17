// Copyright Epic Games, Inc. All Rights Reserved.
 
#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Input/Reply.h"
#include "Layout/Margin.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/SlateDelegates.h"
#include "Framework/Text/TextLayout.h"

class FPaintArgs;
class FSlateWindowElementList;
class FSlateTextBlockLayout;
class IBreakIterator;
enum class ETextShapingMethod : uint8;

namespace ETextRole
{
	enum Type
	{
		Custom,
		ButtonText,
		ComboText
	};
}


/**
 * A simple static text widget
 */
class SLATE_API STextBlock : public SLeafWidget
{
	SLATE_DECLARE_WIDGET(STextBlock, SLeafWidget)

public:

	SLATE_BEGIN_ARGS( STextBlock )
		: _Text()
		, _TextStyle( &FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "NormalText" ) )
		, _Font()
		, _StrikeBrush()
		, _ColorAndOpacity()
		, _ShadowOffset()
		, _ShadowColorAndOpacity()
		, _HighlightColor()
		, _HighlightShape()
		, _HighlightText()
		, _WrapTextAt(0.0f)
		, _AutoWrapText(false)
		, _WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)
		, _TransformPolicy()
		, _Margin()
		, _LineHeightPercentage(1.0f)
		, _Justification(ETextJustify::Left)
		, _MinDesiredWidth(0.0f)
		, _TextShapingMethod()
		, _TextFlowDirection()
		, _LineBreakPolicy()
		, _OverflowPolicy()
		, _SimpleTextMode(false)
		{
			_Clipping = EWidgetClipping::OnDemand;
		}

		/** The text displayed in this text block */
		SLATE_ATTRIBUTE( FText, Text )

		/** Pointer to a style of the text block, which dictates the font, color, and shadow options. */
		SLATE_STYLE_ARGUMENT( FTextBlockStyle, TextStyle )

		/** Sets the font used to draw the text */
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )

		/** Sets the brush used to strike through the text */
		SLATE_ATTRIBUTE( const FSlateBrush*, StrikeBrush )

		/** Text color and opacity */
		SLATE_ATTRIBUTE( FSlateColor, ColorAndOpacity )

		/** Drop shadow offset in pixels */
		SLATE_ATTRIBUTE( FVector2D, ShadowOffset )

		/** Shadow color and opacity */
		SLATE_ATTRIBUTE( FLinearColor, ShadowColorAndOpacity )

		/** The color used to highlight the specified text */
		SLATE_ATTRIBUTE( FLinearColor, HighlightColor )

		/** The brush used to highlight the specified text */
		SLATE_ATTRIBUTE( const FSlateBrush*, HighlightShape )

		/** Highlight this text in the text block */
		SLATE_ATTRIBUTE( FText, HighlightText )

		/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
		SLATE_ATTRIBUTE( float, WrapTextAt )

		/** Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
		    in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial 
			desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout. */
		SLATE_ATTRIBUTE( bool, AutoWrapText )

		/** The wrapping policy to use */
		SLATE_ATTRIBUTE( ETextWrappingPolicy, WrappingPolicy )

		/** The transform policy to use */
		SLATE_ATTRIBUTE( ETextTransformPolicy, TransformPolicy )

		/** The amount of blank space left around the edges of text area. */
		SLATE_ATTRIBUTE( FMargin, Margin )

		/** The amount to scale each lines height by. */
		SLATE_ATTRIBUTE( float, LineHeightPercentage )

		/** How the text should be aligned with the margin. */
		SLATE_ATTRIBUTE( ETextJustify::Type, Justification )

		/** Minimum width that a text block should be */
		SLATE_ATTRIBUTE( float, MinDesiredWidth )

		/** Which text shaping method should we use? (unset to use the default returned by GetDefaultTextShapingMethod) */
		SLATE_ARGUMENT( TOptional<ETextShapingMethod>, TextShapingMethod )
		
		/** Which text flow direction should we use? (unset to use the default returned by GetDefaultTextFlowDirection) */
		SLATE_ARGUMENT( TOptional<ETextFlowDirection>, TextFlowDirection )

		/** The iterator to use to detect appropriate soft-wrapping points for lines (or null to use the default) */
		SLATE_ARGUMENT( TSharedPtr<IBreakIterator>, LineBreakPolicy )

		/** Determines what happens to text that is clipped and doesn't fit within the clip rect for this widget */
		SLATE_ARGUMENT(TOptional<ETextOverflowPolicy>, OverflowPolicy)

		/**
		 * If this is enabled, text shaping, wrapping, justification are disabled in favor of much faster text layout and measurement.
		 * This feature is suitable for numbers and text that changes often and impact performance.
		 * Enabling this setting may cause certain languages (such as Right to left languages) to not display properly.
		 */
		SLATE_ARGUMENT( bool, SimpleTextMode )

		/** Called when this text is double clicked */
		SLATE_EVENT(FPointerEventHandler, OnDoubleClicked)

	SLATE_END_ARGS()

	/** Constructor */
	STextBlock();

	/** Destructor */
	~STextBlock();

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Gets the text assigned to this text block
	 *
	 * @return	This text block's text string
	 */
	const FText& GetText() const
	{
		if (bIsAttributeBoundTextBound)
		{
			STextBlock& MutableSelf = const_cast<STextBlock&>(*this);
			MutableSelf.BoundText.UpdateNow(MutableSelf);
		}
		return BoundText.Get();
	}
	
public:
	/** Sets the text for this text block */
	void SetText(TAttribute<FText> InText);

	/** Sets the highlight text for this text block */
	void SetHighlightText(TAttribute<FText> InText);

	/** Sets the font used to draw the text	*/
	void SetFont(TAttribute<FSlateFontInfo> InFont);

	/** Sets the brush used to strike through the text */
	void SetStrikeBrush(TAttribute<const FSlateBrush*> InStrikeBrush);

	/** See ColorAndOpacity attribute */
	void SetColorAndOpacity(TAttribute<FSlateColor> InColorAndOpacity);

	/** See TextStyle argument */
	void SetTextStyle(const FTextBlockStyle* InTextStyle);

	/** See TextShapingMethod attribute */
	void SetTextShapingMethod(const TOptional<ETextShapingMethod>& InTextShapingMethod);

	/** See TextFlowDirection attribute */
	void SetTextFlowDirection(const TOptional<ETextFlowDirection>& InTextFlowDirection);

	/** See WrapTextAt attribute */
	void SetWrapTextAt(TAttribute<float> InWrapTextAt);

	/** See AutoWrapText attribute */
	void SetAutoWrapText(TAttribute<bool> InAutoWrapText);

	/** Set WrappingPolicy attribute */
	void SetWrappingPolicy(TAttribute<ETextWrappingPolicy> InWrappingPolicy);

	/** Set TransformPolicy attribute */
	void SetTransformPolicy(TAttribute<ETextTransformPolicy> InTransformPolicy);

	/** Get TransformPolicy attribute */
	UE_DEPRECATED(5.0, "GetTransformPolicy is not accessible anymore since it's attribute value may not have been updated yet.")
	ETextTransformPolicy GetTransformPolicy() const { return GetTransformPolicyImpl(); }

	/** Sets the overflow policy for this text block */
	void SetOverflowPolicy(TOptional<ETextOverflowPolicy> InOverflowPolicy);

	/** See ShadowOffset attribute */
	void SetShadowOffset(TAttribute<FVector2D> InShadowOffset);

	/** See ShadowColorAndOpacity attribute */
	void SetShadowColorAndOpacity(TAttribute<FLinearColor> InShadowColorAndOpacity);

	/** See HighlightColor attribute */
	void SetHighlightColor(TAttribute<FLinearColor> InHighlightColor);
	
	/** See HighlightShape attribute */
	void SetHighlightShape(TAttribute<const FSlateBrush*> InHighlightShape);

	/** See MinDesiredWidth attribute */
	void SetMinDesiredWidth(TAttribute<float> InMinDesiredWidth);

	/** See LineHeightPercentage attribute */
	void SetLineHeightPercentage(TAttribute<float> InLineHeightPercentage);

	/** See Margin attribute */
	void SetMargin(TAttribute<FMargin> InMargin);

	/** See Justification attribute */
	void SetJustification(TAttribute<ETextJustify::Type> InJustification);

	// SWidget interface
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
#if WITH_ACCESSIBILITY
	virtual TSharedRef<FSlateAccessibleWidget> CreateAccessibleWidget() override;
	virtual TOptional<FText> GetDefaultAccessibleText(EAccessibleType AccessibleType = EAccessibleType::Main) const override;
#endif
	// End of SWidget interface

private:
	/** Get the computed text style to use with the text marshaller */
	FTextBlockStyle GetComputedTextStyle() const;

	/** Gets the current foreground color */
	FSlateColor GetColorAndOpacity() const;

	/** Gets the current font */
	FSlateFontInfo GetFont() const;

	/** Gets the current strike brush */
	const FSlateBrush* GetStrikeBrush() const;

	/** Get TransformPolicy attribute */
	ETextTransformPolicy GetTransformPolicyImpl() const;
	
	/** Gets the current shadow offset */
	FVector2D GetShadowOffset() const;

	/** Gets the current shadow color and opacity */
	FLinearColor GetShadowColorAndOpacity() const;

	/** Gets the current highlight color */
	FSlateColor GetHighlightColor() const;

	/** Gets the current highlight shape */
	const FSlateBrush* GetHighlightShape() const;

	/** Call to invalidate this text block */
	void InvalidateText(EInvalidateWidgetReason InvalidateReason);

private:


private:
	/** The text displayed in this text block */
	TSlateAttribute<FText> BoundText;

	/** The wrapped layout for this text block */
	TUniquePtr< FSlateTextBlockLayout > TextLayoutCache;

	/** Default style used by the TextLayout */
	FTextBlockStyle TextStyle;

	/** Sets the font used to draw the text */
	TSlateAttribute<FSlateFontInfo> Font;

	/** Sets the brush used to strike through the text */
	TSlateAttribute<const FSlateBrush*> StrikeBrush;

	/** Text color and opacity */
	TSlateAttribute<FSlateColor> ColorAndOpacity;

	/** Drop shadow offset in pixels */
	TSlateAttribute<FVector2D> ShadowOffset;

	/** Shadow color and opacity */
	TSlateAttribute<FLinearColor> ShadowColorAndOpacity;

	/** The color used to highlight the specified text */
	TSlateAttribute<FLinearColor> HighlightColor;

	/** The brush used to highlight the specified text */
	TSlateAttribute<const FSlateBrush*> HighlightShape;

	/** Highlight this text in the TextBlock */
	TSlateAttribute<FText> HighlightText;

	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	TSlateAttribute<float> WrapTextAt;

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	TSlateAttribute<bool> AutoWrapText;

	/** The wrapping policy we're using */
	TSlateAttribute<ETextWrappingPolicy> WrappingPolicy;

	/** The transform policy we're using */
	TSlateAttribute<ETextTransformPolicy> TransformPolicy;

	/** The amount of blank space left around the edges of text area. */
	TSlateAttribute<FMargin> Margin;

	/** The amount to scale each lines height by. */
	TSlateAttribute<ETextJustify::Type> Justification;

	/** How the text should be aligned with the margin. */
	TSlateAttribute<float> LineHeightPercentage;

	/** Prevents the text block from being smaller than desired in certain cases (e.g. when it is empty) */
	TSlateAttribute<float> MinDesiredWidth;

	/** If this is enabled, text shaping, wrapping, justification are disabled in favor of much faster text layout and measurement. */
	mutable TOptional<FVector2f> CachedSimpleDesiredSize;

	/** Flags used to check if the SlateAttribute is set. */
	union
	{
		struct 
		{
			uint16 bIsAttributeBoundTextBound : 1;
			uint16 bIsAttributeFontSet : 1;
			uint16 bIsAttributeStrikeBrushSet : 1;
			uint16 bIsAttributeColorAndOpacitySet : 1;
			uint16 bIsAttributeShadowOffsetSet : 1;
			uint16 bIsAttributeShadowColorAndOpacitySet : 1;
			uint16 bIsAttributeHighlightColorSet : 1;
			uint16 bIsAttributeHighlightShapeSet : 1;
			uint16 bIsAttributeWrapTextAtSet : 1;
			uint16 bIsAttributeTransformPolicySet : 1;
		};
		uint16 Union_Flags;
	};

	bool bSimpleTextMode;
};
