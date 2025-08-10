#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Misc/CommonCommands.h"
#include <Widgets/Input/SSearchBox.h>
#include <Styling/StyleColors.h>
#include "Common/Util/Math.h"

namespace FW
{

	SGraphPanel::SGraphPanel() 
		: Nodes(this), ViewOffset(0), MousePos(0),
		ZoomValue(0)
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
		Links.Empty();
		Nodes.Empty();
		SelectedNodes.Empty();
		MenuNodeItems.Empty();
		ViewOffset = 0;
		MousePos = 0;
		ZoomValue = 0;
	}

	void SGraphPanel::AddNode(ObjectPtr<GraphNode> NewNodeData)
	{
		NewNodeData->InitPins();
		NewNodeData->SetOuter(GraphData);
		NewNodeData->Position = PanelCoordToGraphCoord(MousePos);
		
		ShObjectOp* Op = GetShObjectOp(NewNodeData);
		Op->OnSelect(NewNodeData);

		auto NodeWidget = NewNodeData->CreateNodeWidget(this);
		Nodes.Add(NodeWidget);
		
		ClearSelectedNode();
		GraphData->AddNode(NewNodeData);
		AddSelectedNode(NodeWidget);
		GraphData->MarkDirty();
	}

	void SGraphPanel::AddLink(SGraphPin* Output, SGraphPin* Input)
	{
		if (Input->PinData->Accept(Output->PinData))
		{
			SGraphNode* OutputOwner = Output->Owner;
			SGraphNode* InputOwner = Input->Owner;
			OutputOwner->NodeData->OutPinToInPin.AddUnique(Output->PinData->GetGuid(), Input->PinData->GetGuid());
			OutputOwner->AddDep(InputOwner);
			Links.AddUnique(Output, Input);
		}
	}

	void SGraphPanel::RemoveLink(SGraphPin* Input)
	{
		if (auto Key = Links.FindKey(Input))
		{
			SGraphPin* Output = *Key;
			Links.Remove(Output, Input);
			Output->Owner->RemoveDep(Input->Owner);
			auto Kkey = Output->Owner->NodeData->OutPinToInPin.FindKey(Input->PinData->GetGuid());
			Output->Owner->NodeData->OutPinToInPin.Remove(*Kkey, Input->PinData->GetGuid());
            Input->PinData->Refuse();
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

	SGraphPin* SGraphPanel::GetGraphPin(FGuid PinId)
	{
		for (int i = 0; i < Nodes.Num(); i++)
		{
			for (auto PinPtr: Nodes[i]->Pins)
			{
				if (PinPtr->PinData->GetGuid() == PinId)
				{
					return PinPtr;
				}
			}
		}
		return nullptr;
	}

	SGraphPin* SGraphPanel::GetOuputPinInLink(SGraphPin* InputPin) const
	{
		auto KeyPtr = Links.FindKey(InputPin);
		if (KeyPtr)
		{
			return *KeyPtr;
		}
		return nullptr;
	}

	void SGraphPanel::SetGraphData(Graph* InGraphData)
	{
		Clear();
		GraphData = InGraphData;
		if (GraphData)
		{
			for (auto& NodeData : GraphData->GetNodes())
			{
				auto NodeWidget = NodeData->CreateNodeWidget(this);
				Nodes.Add(NodeWidget);
			}

			for (auto& NodeData : GraphData->GetNodes())
			{
				for (auto [OutPinId, InPinId] : NodeData->OutPinToInPin)
				{
					AddLink(GetGraphPin(OutPinId), GetGraphPin(InPinId));
				}
			}
			if(auto* NodeMetaTypes = RegisteredNodes.Find(GetRegisteredName(GraphData->DynamicMetaType())))
			{
				for (auto NodeMetaType : *NodeMetaTypes)
				{
					FText NodeTitle = static_cast<GraphNode*>(NodeMetaType->GetDefaultObject())->ObjectName;
					MenuNodeItems.Add(MakeShared<FText>(MoveTemp(NodeTitle)));
				}
			}
		}
	}

	void SGraphPanel::AddSelectedNode(TSharedRef<SGraphNode> InNode)
	{
		if (!SelectedNodes.Contains(&*InNode))
		{
			//Layer order
			Nodes.Remove(InNode);
			Nodes.Add(InNode);
            ObjectPtr<GraphNode> NodeData = GraphData->GetNode(InNode->NodeData->GetGuid());
			GraphData->RemoveNode(InNode->NodeData->GetGuid());
			GraphData->AddNode(NodeData);

			ShObjectOp* Op = GetShObjectOp(NodeData);
			Op->OnSelect(NodeData);
			SelectedNodes.Add(&*InNode);
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

	FReply SGraphPanel::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && MouseEvent.IsControlDown())
		{
			CutLineStart = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			CutLineEnd = *CutLineStart;
			return FReply::Handled().CaptureMouse(AsShared()).LockMouseToWidget(AsShared());
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			MarqueeStart = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			MarqueeEnd = *MarqueeStart;
			return FReply::Handled().CaptureMouse(AsShared()).LockMouseToWidget(AsShared());
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			ClearSelectedNode();
			if (MarqueeStart)
			{
				Vector2D MarqueeUpperLeft = { FMath::Min((*MarqueeStart).x, MarqueeEnd.x), FMath::Min((*MarqueeStart).y, MarqueeEnd.y) };
				Vector2D MarqueeBottomRight = { FMath::Max((*MarqueeStart).x, MarqueeEnd.x), FMath::Max((*MarqueeStart).y, MarqueeEnd.y) };
				TArray<TSharedRef<SGraphNode>> SelectedNodeWidgets;
				for (int i = 0; i < Nodes.Num(); i++)
				{
					TSharedRef<SGraphNode> NodeWidget = Nodes[i];
					Vector2D NodeSize = NodeWidget->GetTickSpaceGeometry().GetLocalSize();
					Vector2D NodeUpperLeft = MyGeometry.AbsoluteToLocal(NodeWidget->GetTickSpaceGeometry().GetAbsolutePosition());
					Vector2D NodeBootomRight = NodeUpperLeft + NodeSize;
					if (FSlateRect::DoRectanglesIntersect({ MarqueeUpperLeft, MarqueeBottomRight }, { NodeUpperLeft, NodeBootomRight }))
					{
						SelectedNodeWidgets.Add(NodeWidget);
					}
				}
				for (auto NodeWidget : SelectedNodeWidgets)
				{
					AddSelectedNode(NodeWidget);
				}
				MarqueeStart.Reset();
			}
			return FReply::Handled().ReleaseMouseCapture().ReleaseMouseLock();
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (CutLineStart)
			{
                for (auto [OuputPin, InputPin] : Links)
				{
					Vector2D C0 = MyGeometry.AbsoluteToLocal(OuputPin->GetTickSpaceGeometry().GetAbsolutePositionAtCoordinates({0.5, 0.5}));
					Vector2D C3 = MyGeometry.AbsoluteToLocal(InputPin->GetTickSpaceGeometry().GetAbsolutePositionAtCoordinates({0.5, 0.5}));
					double Offset = FMath::Abs(C0.x - C3.x) / 2;
					Vector2D C1 = {C0.x + Offset, C0.y};
					Vector2D C2 = { C3.x - Offset, C3.y };

					if (LineBezierIntersection(*CutLineStart, CutLineEnd, C0, C1, C2, C3))
					{
                        RemoveLink(InputPin);
						
						GraphData->MarkDirty();
					}
				}
				CutLineStart.Reset();
				return FReply::Handled().ReleaseMouseLock();
			}
			else if (GraphData)
			{
				FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
				FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, CreateContextMenu(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
				return FReply::Handled();
			}
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
		if (bIsRightMouseButtonDown)
		{
			CutLineEnd = MousePos;
			return FReply::Handled();
		}
		else if (bIsLeftMouseButtonDown)
		{
			if (MarqueeStart)
			{
				MarqueeEnd = MousePos;
			}
			else if(!SelectedNodes.IsEmpty())
			{
				for (SGraphNode* Node : SelectedNodes)
				{
					Node->NodeData->Position += Offset;
				}
                if(FMath::Abs(DeltaPos.x) > 0.8 || FMath::Abs(DeltaPos.y) > 0.8)
                {
                    GraphData->MarkDirty();
                }
			}
			return FReply::Handled();
		}
		else if (bIsMiddleMouseButtonDown)
		{
			ViewOffset -= Offset;
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (PreviewStart || CutLineStart) {
			return FReply::Unhandled();
		}
		ZoomValue += MouseEvent.GetWheelDelta() * 0.1f;
		ZoomValue = FMath::Clamp(ZoomValue, -0.5f, 1.0f);
		return FReply::Handled();
	}

	void SGraphPanel::OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent)
	{
		if(GraphData)
		{
			TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
			GraphData->OnDragEnter(DragDropOp);
		}
	}

	void SGraphPanel::OnDragLeave(FDragDropEvent const& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
	}

	FReply SGraphPanel::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		MousePos = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp->IsOfType<GraphDragDropOp>())
		{
			PreviewEnd = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
			return FReply::Handled().LockMouseToWidget(AsShared());
		}
		return FReply::Unhandled();
	}

	FReply SGraphPanel::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		PreviewStart.Reset();
		if(GraphData)
		{
			TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
			GraphData->OnDrop(DragDropOp);
		}
		return FReply::Handled().ReleaseMouseLock();
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
		const int32 PanelLayer = LayerId;

        const FSlateBrush* BackGround = FAppStyle::Get().GetBrush("Brushes.Recessed");
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			PanelLayer,
			AllottedGeometry.ToPaintGeometry(),
            BackGround,
			ESlateDrawEffect::None,
            BackGround->GetTint(InWidgetStyle)
		);

		const FPaintArgs NewArgs = Args.WithNewParent(this);
		const bool bShouldBeEnabled = ShouldBeEnabled(bParentEnabled);
		int32 MaxTopNodeLayer = PanelLayer;
		for (int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
		{
			FArrangedWidget& CurWidget = ArrangedChildren[ChildIndex];
			auto CurNodeWidget = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
			const int32 NodeLayer = MaxTopNodeLayer + 1;
			
			int32 TopNodeLayer = CurWidget.Widget->Paint(NewArgs, CurWidget.Geometry, MyCullingRect, OutDrawElements, NodeLayer, InWidgetStyle, bShouldBeEnabled);
			MaxTopNodeLayer = FMath::Max(MaxTopNodeLayer, TopNodeLayer);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				NodeLayer,
				CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{13,13}),
				FAppCommonStyle::Get().GetBrush("Graph.NodeShadow"),
				ESlateDrawEffect::None
			);
			
			if(CurNodeWidget->NodeData->IsDebugging)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					NodeLayer,
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 2, 2}),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None,
					FLinearColor::Yellow
				);
			}
            else if(CurNodeWidget->NodeData->AnyError)
            {
                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    NodeLayer,
                    CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 2, 2}),
                    FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
                    ESlateDrawEffect::None,
                    FLinearColor::Red
                );
            }
			else if (SelectedNodes.Contains( &*CurNodeWidget))
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					NodeLayer,
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 2, 2}),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None
				);
			}
		}

		if (PreviewStart)
		{
			DrawConnection(AllottedGeometry.ToPaintGeometry(), OutDrawElements, PanelLayer, PreviewStartDir, *PreviewStart, PreviewEnd);
		}

		for (auto [OutPin, InPin] : Links)
		{
			Vector2D OutPos = AllottedGeometry.AbsoluteToLocal(OutPin->GetPaintSpaceGeometry().GetAbsolutePositionAtCoordinates({ 0.5, 0.5 }));
			Vector2D InPos = AllottedGeometry.AbsoluteToLocal(InPin->GetPaintSpaceGeometry().GetAbsolutePositionAtCoordinates({ 0.5, 0.5 }));
			DrawConnection(AllottedGeometry.ToPaintGeometry(), OutDrawElements, PanelLayer, PinDirection::Output, OutPos, InPos);
		}

		if (CutLineStart)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, MaxTopNodeLayer, AllottedGeometry.ToPaintGeometry(), {*CutLineStart, CutLineEnd}, ESlateDrawEffect::None, FLinearColor::Red);
		}

		if (MarqueeStart)
		{
			Vector2D MarqueeSize = { FMath::Abs(MarqueeEnd.x - (*MarqueeStart).x), FMath::Abs(MarqueeEnd.y - (*MarqueeStart).y)};
			Vector2D UpperLeft = { FMath::Min((*MarqueeStart).x, MarqueeEnd.x), FMath::Min((*MarqueeStart).y, MarqueeEnd.y) };
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				MaxTopNodeLayer,
				AllottedGeometry.ToPaintGeometry(MarqueeSize, FSlateLayoutTransform(UpperLeft)),
				FAppCommonStyle::Get().GetBrush("Graph.Selector")
			);
		}
        
        if(GraphData && GraphData->AnyError)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                MaxTopNodeLayer,
                AllottedGeometry.ToPaintGeometry(),
                FAppCommonStyle::Get().GetBrush("Graph.Shadow"),
                ESlateDrawEffect::None,
                FLinearColor::Red
            );
        }

		return MaxTopNodeLayer;
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
			return CurNode->ObjectName.EqualTo(*InSelectedItem);
		});

		ObjectPtr<GraphNode> NewNodeData = static_cast<GraphNode*>(DefaultNodeData->DynamicMetaType()->Construct());
		AddNode(NewNodeData);

		FSlateApplication::Get().DismissAllMenus();
	}

	void SGraphPanel::ClearSelectedNode()
	{
		for (auto It = SelectedNodes.CreateIterator(); It; It++)
		{
			ShObjectOp* Op = GetShObjectOp((*It)->NodeData);
			Op->OnCancelSelect((*It)->NodeData);
			It.RemoveCurrent();
		}
	}

	void SGraphPanel::DeleteSelectedNodes()
	{
		for (auto Node : SelectedNodes)
		{
			DeleteNode(Node);
		}
	}

	void SGraphPanel::DeleteNode(SGraphNode* Node)
	{
		for (auto [OuputPin, InputPin] : Links)
		{
			if (OuputPin->Owner == Node || InputPin->Owner == Node)
			{
                RemoveLink(InputPin);
			}
		}

		ShObjectOp* Op = GetShObjectOp(Node->NodeData);
		Op->OnCancelSelect(Node->NodeData);
		
		GraphData->RemoveNode(Node->NodeData->GetGuid());

		int RemoveIndex = -1;
		for (int i = 0; i < Nodes.Num(); i++)
		{
			if (&*Nodes[i] == Node)
			{
				RemoveIndex = i;
				break;
			}
		}
		
		check(RemoveIndex != -1);
		Nodes.RemoveAt(RemoveIndex);
		
		SelectedNodes.Remove(Node);
		GraphData->MarkDirty();
	}


}


