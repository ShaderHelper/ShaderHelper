#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetObject/Graph.h"

namespace FRAMEWORK
{

	SGraphPanel::SGraphPanel() : Nodes(this), ViewOffset(0)
	{

	}

	void SGraphPanel::Construct(const FArguments& InArgs)
	{
		GraphData = InArgs._GraphData;
		if (GraphData)
		{
			for (auto& NodeData : GraphData->NodeDatas)
			{
				Nodes.Add(NodeData->CreateNodeWidget());
			}
		}
	}

	void SGraphPanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
	{
		for (int32 ChildIndex = 0; ChildIndex < Nodes.Num(); ++ChildIndex)
		{
			const TSharedRef<SGraphNode>& Node = Nodes[ChildIndex];
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Node, Node->GetPosition() - ViewOffset, Node->GetDesiredSize(), 1.0f));
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

	//FReply SGraphPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	//{

	//}

	//FReply SGraphPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	//{

	//}

	void SGraphPanel::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
	{

	}

	//FReply SGraphPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	//{

	//}

	//FReply SGraphPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	//{

	//}

	int32 SGraphPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppCommonStyle::Get().GetBrush("Graph.Background"),
			ESlateDrawEffect::None
		);

		return LayerId;
	}

}


