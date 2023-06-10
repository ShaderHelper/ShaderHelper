#pragma once
#include <Widgets/Text/SlateEditableTextTypes.h>
namespace SH
{
	class FoldMarkerHighLighter : public ISlateLineHighlighter
	{
	public:
		static TSharedRef<FoldMarkerHighLighter> Create()
		{
			return MakeShareable(new FoldMarkerHighLighter());
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FVector2D Location(Line.Offset.X + OffsetX, Line.Offset.Y + 4);

			FLinearColor BackgroundColorAndOpacity = FLinearColor{0.1f, 0.1f, 0.1f, 0.8f};
			const float InverseScale = Inverse(AllottedGeometry.Scale);

			if (Width > 0.0f)
			{
				// Draw the actual highlight rectangle
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					AllottedGeometry.ToPaintGeometry(TransformVector(InverseScale, FVector2D(Width, FMath::Max(Line.Size.Y, Line.TextHeight) - 6.0f)), FSlateLayoutTransform(TransformPoint(InverseScale, Location))),
					&DefaultStyle.HighlightShape,
					bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
					BackgroundColorAndOpacity
				);
			}

			return LayerId;
		}

	protected:
		FoldMarkerHighLighter()
		{
		}
	};
}