#pragma once
#include "SGraphNode.h"

namespace FRAMEWORK
{
	class FRAMEWORK_API SGraphPanel : public SPanel
	{
	public:
		SLATE_BEGIN_ARGS(SGraphPanel) : _GraphData(nullptr)
			{}
			SLATE_ARGUMENT(class Graph*, GraphData)
		SLATE_END_ARGS()

		SGraphPanel();
		void Construct(const FArguments& InArgs);
	public:
		virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
		virtual FVector2D ComputeDesiredSize(float) const override;
		virtual FChildren* GetChildren() override;

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		//virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		//virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
		//virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		//virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		TSlotlessChildren<SGraphNode> Nodes;
		Vector2D ViewOffset;
	};
	
}


