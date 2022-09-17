// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/ISlateLineHighlighter.h"

class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
struct FGeometry;
struct FTextBlockStyle;

/** Run highlighter used to draw lines */
class SLATE_API ISlateTextLineHighlighter : public ISlateLineHighlighter
{
public:
	virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	static const int32 DefaultZIndex = 1;

protected:
	ISlateTextLineHighlighter(const FSlateBrush& InLineBrush, const FSlateFontInfo& InFontInfo, const FSlateColor InColorAndOpacity, const FVector2D InShadowOffset, const FLinearColor InShadowColorAndOpacity);

	virtual void GetLineMetrics(const float InFontScale, int16& OutLinePos, int16& OutLineThickness) const = 0;

	/** Brush used to draw the line */
	FSlateBrush LineBrush;

	/** Font the underline is associated with */
	FSlateFontInfo FontInfo;

	/** The color to draw the underline (typically matches the text its associated with) */
	FSlateColor ColorAndOpacity;

	/** Offset at which to draw the shadow (if any) */
	FVector2D ShadowOffset;

	/** The color to draw the shadow */
	FLinearColor ShadowColorAndOpacity;
};

/** Run highlighter used to draw underlines */
class SLATE_API FSlateTextUnderlineLineHighlighter : public ISlateTextLineHighlighter
{
public:
	static TSharedRef<FSlateTextUnderlineLineHighlighter> Create(const FSlateBrush& InUnderlineBrush, const FSlateFontInfo& InFontInfo, const FSlateColor InColorAndOpacity, const FVector2D InShadowOffset, const FLinearColor InShadowColorAndOpacity);

protected:
	FSlateTextUnderlineLineHighlighter(const FSlateBrush& InUnderlineBrush, const FSlateFontInfo& InFontInfo, const FSlateColor InColorAndOpacity, const FVector2D InShadowOffset, const FLinearColor InShadowColorAndOpacity);

	virtual void GetLineMetrics(const float InFontScale, int16& OutLinePos, int16& OutLineThickness) const override;
};

/** Run highlighter used to draw strikes */
class SLATE_API FSlateTextStrikeLineHighlighter : public ISlateTextLineHighlighter
{
public:
	static TSharedRef<FSlateTextStrikeLineHighlighter> Create(const FSlateBrush& InUnderlineBrush, const FSlateFontInfo& InFontInfo, const FSlateColor InColorAndOpacity, const FVector2D InShadowOffset, const FLinearColor InShadowColorAndOpacity);

protected:
	FSlateTextStrikeLineHighlighter(const FSlateBrush& InStrikeBrush, const FSlateFontInfo& InFontInfo, const FSlateColor InColorAndOpacity, const FVector2D InShadowOffset, const FLinearColor InShadowColorAndOpacity);

	virtual void GetLineMetrics(const float InFontScale, int16& OutLinePos, int16& OutLineThickness) const override;
};
