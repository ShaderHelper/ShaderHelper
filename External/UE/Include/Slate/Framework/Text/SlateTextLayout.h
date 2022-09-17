// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Styling/SlateTypes.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/SWidget.h"
#include "Layout/Children.h"

class FArrangedChildren;
class FPaintArgs;
class FSlateWindowElementList;

class SLATE_API FSlateTextLayout : public FTextLayout
{
public:

	static TSharedRef< FSlateTextLayout > Create(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle);

	FChildren* GetChildren();

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;

	virtual void EndLayout() override;

	void SetDefaultTextStyle(FTextBlockStyle InDefaultTextStyle);
	const FTextBlockStyle& GetDefaultTextStyle() const;

	void SetIsPassword(const TAttribute<bool>& InIsPassword);

protected:

	FSlateTextLayout(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle);

	virtual int32 OnPaintHighlights(const FPaintArgs& Args, const FTextLayout::FLineView& LineView, const TArray<FLineViewHighlight>& Highlights, const FTextBlockStyle& DefaultTextStyle, const FGeometry& AllottedGeometry, const FSlateRect& ClippingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;

	virtual void AggregateChildren();

	virtual TSharedRef<IRun> CreateDefaultTextRun(const TSharedRef<FString>& NewText, const FTextRange& NewRange) const override;

protected:
	/** Default style used by the TextLayout */
	FTextBlockStyle DefaultTextStyle;

private:

	TSlotlessChildren< SWidget > Children;

	/** This this layout displaying a password? */
	TAttribute<bool> bIsPassword;

	friend class FSlateTextLayoutFactory;
};
