#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Misc/CommonCommands.h"
#include <Widgets/Input/SSearchBox.h>
#include <Styling/StyleColors.h>
#include "Common/Util/Math.h"

namespace FRAMEWORK
{

	SGraphPanel::SGraphPanel() 
		: Nodes(this), ViewOffset(0), MousePos(0),
		ZoomValue(0), SelectedNode(nullptr)
	{

	}

	void SGraphPanel::Construct(const FArguments& InArgs)
	{
		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			CommonCommands::Get().Save,
			FExecuteAction::CreateLambda([this] {
				GraphData->Save();
			})
		);

		SetGraphData(InArgs._GraphData);
	}


	void SGraphPanel::Clear()
	{
		Nodes.Empty();
		MenuNodeItems.Empty();
		ViewOffset = 0;
		MousePos = 0;
		ZoomValue = 0;
		SelectedNode = nullptr;
	}

	TSharedPtr<SGraphNode> SGraphPanel::AddNodeFromData(GraphNode* InNodeData)
	{
		auto NodeWidget = InNodeData->CreateNodeWidget(this);
		Nodes.Add(NodeWidget);
		return NodeWidget;
	}

	void SGraphPanel::AddLink(SGraphPin* Output, SGraphPin* Input)
	{
		SGraphNode* OutputOwner = Output->Owner;
		SGraphNode* InputOwner = Input->Owner;
		OutputOwner->NodeData->OutPinToInPin.Add(Output->PinData->Guid, Input->PinData->Guid);
		
		Links.AddUnique(Output, Input);
	}

	void SGraphPanel::RemoveInputLink(SGraphPin* Input)
	{
		if (auto Key = Links.FindKey(Input))
		{
			Links.Remove(*Key);
		}
	}

	Vector2D SGraphPanel::PanelCoordToGraphCoord(const Vector2D& InCoord) const
	{
		Vector2D Offset = GetPaintSpaceGeometry().GetLocalSize() / 2;
		Vector2D GraphCoord = FSlateLayoutTransform{ 1.0f + ZoomValue, Offset }.Inverse().TransformPoint(InCoord);
		return GraphCoord + ViewOffset;
	}

	Vector2D SGraphPanel::GraphCoordToPanelCoord(const Vector2D& InCoord) const
	{
		Vector2D Offset = GetPaintSpaceGeometry().GetLocalSize() / 2;
		return FSlateLayoutTransform{ 1.0f + ZoomValue, Offset }.TransformPoint(InCoord - ViewOffset);
	}

	void SGraphPanel::SetGraphData(Graph* InGraphData)
	{
		Clear();
		GraphData = InGraphData;
		if (GraphData)
		{
			for (auto& NodeData : GraphData->GetNodes())
			{
				AddNodeFromData(NodeData.Get());
			}

			for (auto NodeMetaType : GraphData->SupportNodes())
			{
				FText NodeTitle = static_cast<GraphNode*>(NodeMetaType->GetDefaultObject())->GetNodeTitle();
				MenuNodeItems.Add(MakeShared<FText>(MoveTemp(NodeTitle)));
			}
		}
	}

	void SGraphPanel::SetSelectedNode(TSharedPtr<SGraphNode> InNode)
	{
		if (SelectedNode != InNode)
		{
			InNode->NodeData->Layer = CurMaxNodeLayer + 1;
			SelectedNode = InNode;
		}
		
	}

	void SGraphPanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
	{
		for (int32 ChildIndex = 0; ChildIndex < Nodes.Num(); ++ChildIndex)
		{
			const TSharedRef<SGraphNode>& Node = Nodes[ChildIndex];;
			Vector2D PanelCoordNode = GraphCoordToPanelCoord(Node->NodeData->Position);
			ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Node, Node->GetDesiredSize(), FSlateLayoutTransform{ 1.0f + ZoomValue, PanelCoordNode }));
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

	FReply SGraphPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && MouseEvent.IsControlDown())
		{
			CutLineStart = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (CutLineStart)
			{
				for (TMultiMap<SGraphPin*, SGraphPin*>::TIterator It = Links.CreateIterator(); It; ++It)
				{
					Vector2D C0 = MyGeometry.AbsoluteToLocal(It.Key()->GetTickSpaceGeometry().GetAbsolutePositionAtCoordinates({0.5, 0.5}));
					Vector2D C3 = MyGeometry.AbsoluteToLocal(It.Value()->GetTickSpaceGeometry().GetAbsolutePositionAtCoordinates({0.5, 0.5}));
					double Offset = FMath::Abs(C0.x - C3.x) / 2;
					Vector2D C1 = {C0.x + Offset, C0.y};
					Vector2D C2 = { C3.x - Offset, C3.y };

					if (LineBezierIntersection(*CutLineStart, CutLineEnd, C0, C1, C2, C3))
					{
						It.RemoveCurrent();
					}
				}
				CutLineStart.Reset();
			}
			else if (GraphData)
			{
				FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
				FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, CreateContextMenu(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
			}

			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
		const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
		const bool bIsRightMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);

		Vector2D CurMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Vector2D DeltaPos = CurMousePos - MousePos;
		MousePos = CurMousePos;

		Vector2D Offset = DeltaPos / (1.0f + ZoomValue);
		if (bIsLeftMouseButtonDown)
		{
			if (SelectedNode.IsValid())
			{
				SelectedNode.Pin()->NodeData->Position += Offset;
				GraphData->MarkDirty();
			}
			return FReply::Handled();
		}
		else if (bIsMiddleMouseButtonDown)
		{
			ViewOffset -= Offset;
			return FReply::Handled();
		}
		else if (bIsRightMouseButtonDown)
		{
			CutLineEnd = MousePos;
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (PreviewStart) {
			return FReply::Unhandled();
		}
		ZoomValue += MouseEvent.GetWheelDelta() * 0.1f;
		ZoomValue = FMath::Clamp(ZoomValue, -0.5f, 1.0f);
		return FReply::Handled();
	}

	FReply SGraphPanel::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp->IsOfType<GraphDragDropOp>())
		{
			PreviewEnd = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		PreviewStart.Reset();
		return FReply::Handled();
	}

	FReply SGraphPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	void SGraphPanel::DrawConnection(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 Layer, PinDirection InStartDir, const Vector2D& Start, const Vector2D& End) const
	{
		double Offset = FMath::Abs(End.x - Start.x) / 2;
		double StartOffset = InStartDir == PinDirection::Output ? Offset : -Offset;
		double EndOffset = -StartOffset;
		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, Layer, PaintGeometry, Start, { Start.x + StartOffset, Start.y }, {End.x + EndOffset, End.y}, End, 2.0f);
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

		const FPaintArgs NewArgs = Args.WithNewParent(this);
		const bool bShouldBeEnabled = ShouldBeEnabled(bParentEnabled);

		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
			auto CurNodeWidget = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
			const int32 NodeLayerId = CurNodeWidget->NodeData->Layer ? *CurNodeWidget->NodeData->Layer : LayerId + 1;
			
			int32 NodeLayer = CurWidget.Widget->Paint(NewArgs, CurWidget.Geometry, MyCullingRect, OutDrawElements, NodeLayerId, InWidgetStyle, bShouldBeEnabled);
			CurMaxNodeLayer = FMath::Max(CurMaxNodeLayer, NodeLayer);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				NodeLayerId,
				CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{13,13}),
				FAppCommonStyle::Get().GetBrush("Graph.NodeShadow"),
				ESlateDrawEffect::None
			);

			if (SelectedNode == CurNodeWidget)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					NodeLayerId,
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 1, 1}),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None
				);
			}
		}

		if (PreviewStart)
		{
			DrawConnection(AllottedGeometry.ToPaintGeometry(), OutDrawElements, LayerId, PreviewStartDir, *PreviewStart, PreviewEnd);
		}

		for (auto [OutPin, InPin] : Links)
		{
			Vector2D OutPos = AllottedGeometry.AbsoluteToLocal(OutPin->GetPaintSpaceGeometry().GetAbsolutePositionAtCoordinates({ 0.5, 0.5 }));
			Vector2D InPos = AllottedGeometry.AbsoluteToLocal(InPin->GetPaintSpaceGeometry().GetAbsolutePositionAtCoordinates({ 0.5, 0.5 }));
			DrawConnection(AllottedGeometry.ToPaintGeometry(), OutDrawElements, LayerId, PinDirection::Output, OutPos, InPos);
		}

		if (CutLineStart)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), {*CutLineStart, CutLineEnd}, ESlateDrawEffect::None, FLinearColor::Red);
		}

		return LayerId;
	}

	TSharedRef<SWidget> SGraphPanel::CreateContextMenu()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSearchBox)
					.DelayChangeNotificationsWhileTyping(false)
			]
			+ SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppCommonStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FStyleColors::Dropdown)
				[
					SNew(SListView<TSharedPtr<FText>>)
						.ListItemsSource(&MenuNodeItems)
						.OnSelectionChanged(this, &SGraphPanel::OnMenuItemSelected)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SGraphPanel::GenerateNodeItems)
				]

			];
	}

	TSharedRef<ITableRow> SGraphPanel::GenerateNodeItems(TSharedPtr<FText> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<TSharedPtr<FText>>, OwnerTable)
			.Padding(5.0f)
			[
				SNew(STextBlock).Text(*Item).MinDesiredWidth(100.0f)
			];
	}

	void SGraphPanel::OnMenuItemSelected(TSharedPtr<FText> InSelectedItem, ESelectInfo::Type SelectInfo)
	{
		GraphNode* DefaultNodeData = GetDefaultObject<GraphNode>([InSelectedItem](GraphNode* CurNode) {
			return CurNode->GetNodeTitle().EqualTo(*InSelectedItem);
		});

		GraphNode* NewNodeData = static_cast<GraphNode*>(DefaultNodeData->DynamicMetaType()->Construct());
		NewNodeData->Position = PanelCoordToGraphCoord(MousePos);

		auto NodeWidget = AddNodeFromData(NewNodeData);
		SetSelectedNode(NodeWidget);
		GraphData->AddNode(MakeShareable(NewNodeData));
		GraphData->MarkDirty();

		FSlateApplication::Get().DismissAllMenus();
	}

	void SGraphPanel::DeleteNode(TSharedPtr<SGraphNode> InNode)
	{
		GraphData->RemoveNode(InNode->NodeData->Guid);
		Nodes.Remove(InNode.ToSharedRef());

		for (TMultiMap<SGraphPin*, SGraphPin*>::TIterator It = Links.CreateIterator(); It; ++It)
		{
			if (It.Key()->Owner == InNode.Get())
			{
				It.RemoveCurrent();
			}
			else if (It.Value()->Owner == InNode.Get())
			{
				It.RemoveCurrent();
			}
		}

		GraphData->MarkDirty();
	}

}


