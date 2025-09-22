#pragma once
#include <Widgets/SViewport.h>

namespace SH
{
	class SShViewport : public SViewport
	{
	public:
		void Construct(const FArguments& InArgs);

		void ResetSpliter();
		void SetSpliterFraction(const TAttribute<float>& Fraction) { SpliterFraction = Fraction; }
		void SetOnSpliterChanged(const TFunction<void(float)>& Handler) { OnSpliterChanged = Handler; }

		virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	protected:
		TAttribute<float> SpliterFraction;
		TFunction<void(float)> OnSpliterChanged;
		bool bIsDraggingSplitter = false;
		double DragOffset = 0.0;
		static constexpr float SplitterHitTestWidth = 5.0f;
	};
}