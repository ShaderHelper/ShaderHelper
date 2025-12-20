#pragma once
#include "App/App.h"

#include <Widgets/Text/SlateEditableTextTypes.h>

namespace SH
{
	class FoldMarkerHighlighter : public ISlateLineHighlighter
	{
	public:
		static TSharedRef<FoldMarkerHighlighter> Create()
		{
			return MakeShareable(new FoldMarkerHighlighter());
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const float InverseScale = Inverse(AllottedGeometry.Scale);
			Location = TransformPoint(InverseScale, FVector2D(Line.Offset.X + OffsetX, Line.Offset.Y + 4));
			Size = TransformVector(InverseScale, FVector2D(Width, FMath::Max(Line.Size.Y, Line.TextHeight) - 6.0f));

			FLinearColor BackgroundColorAndOpacity = FLinearColor{0.1f, 0.1f, 0.1f, 0.8f};

			if (Width > 0.0f)
			{
				// Draw the actual highlight rectangle
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++LayerId,
					AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Location)),
					&DefaultStyle.HighlightShape,
					bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
					BackgroundColorAndOpacity
				);
			}

			return LayerId;
		}

		mutable FVector2D Location;
		mutable FVector2D Size;

	protected:
		FoldMarkerHighlighter()
		{
		}
	};

	class OccurrenceHighlighter : public ISlateLineHighlighter
	{
	public:
		static TSharedRef<OccurrenceHighlighter> Create()
		{
			return MakeShareable(new OccurrenceHighlighter());
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FVector2D Location(Line.Offset.X + OffsetX, Line.Offset.Y);

			FLinearColor BackgroundColorAndOpacity = FLinearColor{ 0.3f, 0.3f, 0.3f, 0.6f };
			const float InverseScale = Inverse(AllottedGeometry.Scale);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				AllottedGeometry.ToPaintGeometry(TransformVector(InverseScale, FVector2D(Width, FMath::Max(Line.Size.Y, Line.TextHeight))), FSlateLayoutTransform(TransformPoint(InverseScale, Location))),
				&DefaultStyle.HighlightShape,
				bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
				BackgroundColorAndOpacity
			);

			return LayerId;
		}

	protected:
		OccurrenceHighlighter() = default;
	};

	class BracketHighlighter : public ISlateLineHighlighter
	{
	public:
		static TSharedRef<BracketHighlighter> Create()
		{
			return MakeShareable(new BracketHighlighter());
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FVector2D Location(Line.Offset.X + OffsetX, Line.Offset.Y);

			FLinearColor BackgroundColorAndOpacity = FLinearColor{ 0.4f, 0.4f, 0.4f, 1.0f };
			const float InverseScale = Inverse(AllottedGeometry.Scale);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				AllottedGeometry.ToPaintGeometry(TransformVector(InverseScale, FVector2D(Width, FMath::Max(Line.Size.Y, Line.TextHeight))), FSlateLayoutTransform(TransformPoint(InverseScale, Location))),
				FShaderHelperStyle::Get().GetBrush(TEXT("ShaderEditor.Border")),
				ESlateDrawEffect::None,
				BackgroundColorAndOpacity
			);

			return LayerId;
		}

	protected:
		BracketHighlighter() = default;
	};


	class CursorHighlighter : public SlateEditableTextTypes::FCursorLineHighlighter
	{
	public:
		static TSharedRef<CursorHighlighter> Create(const SlateEditableTextTypes::FCursorInfo* InCursorInfo)
		{
			return MakeShareable(new CursorHighlighter(InCursorInfo));
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			FTextLayout::FLineView NewLine = Line;
			NewLine.Offset.X += OffsetX;
            
			float Scale = Inverse(AllottedGeometry.Scale);
			ScaledLineHeight = Line.TextHeight * Scale;
			ScaledCursorPos = NewLine.Offset * Scale;

			if (CursorPos.IsZero()) {
				CursorPos = NewLine.Offset;
			}

			//not blinking
			if (FVector2D::Distance(CursorPos, NewLine.Offset) > 0.1f)
			{
				//Already ensure the cursorinfo do not pointer to a const object.
				const_cast<SlateEditableTextTypes::FCursorInfo*>(CursorInfo)->UpdateLastCursorInteractionTime();
			}

			if (FVector2D::Distance(CursorPos, NewLine.Offset) < 20.0f)
			{
				float Speed = 20.0f;
				//FSlateApplication::Get().GetDeltaTime() will get a strange value here
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
        
    public:
		mutable FVector2D CursorPos = FVector2D::Zero();

		mutable float ScaledLineHeight = 0;
		mutable FVector2D ScaledCursorPos = FVector2D::Zero();
	};
}
