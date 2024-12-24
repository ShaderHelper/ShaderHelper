#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetObject/Graph.h"

namespace FRAMEWORK
{

	SGraphPanel::SGraphPanel() : Nodes(this), ViewOffset(0), ZoomValue(0), SelectedNode(nullptr)
	{

	}

	void SGraphPanel::Construct(const FArguments& InArgs)
	{
		SetGraphData(InArgs._GraphData);
	}

	void SGraphPanel::SetGraphData(Graph* InGraphData)
	{
		GraphData = InGraphData;
		if (GraphData)
		{
			Nodes.Empty();
			for (auto& NodeData : GraphData->GetNodes())
			{
				auto NodeWidget = NodeData->CreateNodeWidget();
				NodeWidget->SetOwner(this);
				Nodes.Add(MoveTemp(NodeWidget));
			}
		}
	}

	void SGraphPanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
	{
		for (int32 ChildIndex = 0; ChildIndex < Nodes.Num(); ++ChildIndex)
		{
			const TSharedRef<SGraphNode>& Node = Nodes[ChildIndex];
			if(!Node->NodeData->Position)
			{
				Node->NodeData->Position = AllottedGeometry.GetLocalSize() / 2;
			}
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Node, Node->GetDesiredSize(), FSlateLayoutTransform{ 1.0f + ZoomValue, *Node->NodeData->Position - ViewOffset}));
		}
	}

	FVector2D SGraphPanel::ComputeDesiredSize(float) const
	{
		return FVector2D(160.0f, 120.0f);
	}

	FChildren* SGraphPanel::GetChildren()
	{
		return &Nodes;
	}

	void SGraphPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{

	}

	//FReply SGraphPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	//{

	//}

	void SGraphPanel::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
	{

	}

	FReply SGraphPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
		const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
		const Vector2D CursorDelta = MouseEvent.GetCursorDelta();

		FVector2D CurMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		static FVector2D LastMousePos = FVector2D::Zero();
		Vector2D DeltaPos = CurMousePos - LastMousePos;
		LastMousePos = CurMousePos;

		if (bIsLeftMouseButtonDown)
		{
			if (SelectedNode)
			{
				*SelectedNode->NodeData->Position += DeltaPos;
			}
			return FReply::Handled();
		}
		else if (bIsMiddleMouseButtonDown)
		{
			ViewOffset -= CursorDelta;
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		ZoomValue += MouseEvent.GetWheelDelta() * 0.1f;
		ZoomValue = FMath::Clamp(ZoomValue, -0.5f, 1.0f);
		return FReply::Handled();
	}

	int32 SGraphPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		ArrangeChildren(AllottedGeometry, ArrangedChildren);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppCommonStyle::Get().GetBrush("Graph.Background"),
			ESlateDrawEffect::None
		);

		const int32 NodeLayerId = LayerId + 1;
		const FPaintArgs NewArgs = Args.WithNewParent(this);
		const bool bShouldBeEnabled = ShouldBeEnabled(bParentEnabled);

		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
			CurWidget.Widget->Paint(NewArgs, CurWidget.Geometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bShouldBeEnabled);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{12,12}),
				FAppCommonStyle::Get().GetBrush("Graph.NodeShadow"),
				ESlateDrawEffect::None
			);

			SGraphNode* CurNodeWidget = &*StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
			if (SelectedNode == CurNodeWidget)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 1.2,1.2}),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None
				);
			}
		}

		return LayerId;
	}

}


