#pragma once
#include <Widgets/Text/SlateEditableTextTypes.h>
#include "App/App.h"

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

	class CursorHightLighter : public SlateEditableTextTypes::FCursorLineHighlighter
	{
	public:
		static TSharedRef<CursorHightLighter> Create(const SlateEditableTextTypes::FCursorInfo* InCursorInfo)
		{
			return MakeShareable(new CursorHightLighter(InCursorInfo));
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			FTextLayout::FLineView NewLine = Line;
			NewLine.Offset.X += OffsetX;

			if (CursorPos.IsZero()) {
				CursorPos = NewLine.Offset;
			}

			//not blinking
			if (FVector2D::Distance(CursorPos, NewLine.Offset) > 0.05f)
			{
				//Already ensure the cursorinfo do not pointer to a const object.
				const_cast<SlateEditableTextTypes::FCursorInfo*>(CursorInfo)->UpdateLastCursorInteractionTime();
			}

			if (FVector2D::Distance(CursorPos, NewLine.Offset) < 20.0f)
			{
				float Speed = 20.0f;
				//FSlateApplication::Get().GetDeltaTime() will get a strange value here, which may be connected with hook.
				CursorPos = FMath::LerpStable(CursorPos, NewLine.Offset, FW::GApp->GetDeltaTime() * Speed);
			}
			else
			{
				CursorPos = NewLine.Offset;
			}

			NewLine.Offset = CursorPos;
			SlateEditableTextTypes::FCursorLineHighlighter::OnPaint(Args, NewLine, 0, Width, DefaultStyle, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
			return LayerId;
		}

	protected:
		using FCursorLineHighlighter::FCursorLineHighlighter;

		mutable FVector2D CursorPos = FVector2D::Zero();
	};
}