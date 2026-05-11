#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "AssetObject/Graph.h"
#include "Editor/GraphEditorCommands.h"
#include "Common/Util/Math.h"

#include <Widgets/Input/SSearchBox.h>
#include <Styling/StyleColors.h>
#include <Fonts/FontMeasure.h>
#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>

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
			GraphEditorCommands::Get().Save,
			FExecuteAction::CreateLambda([this] {
				GraphData->Save();
			})
		);
		UICommandList->MapAction(
			GraphEditorCommands::Get().CutLine,
			FExecuteAction::CreateLambda([this] {
				CutLineStart = MousePos;
				CutLineEnd = *CutLineStart;
			})
		);
		UICommandList->MapAction(
			GraphEditorCommands::Get().Delete,
			FExecuteAction::CreateLambda([this] {
				ScopedTransaction Transaction(this);
				DeleteSelectedNodes();
			})
		);
		UICommandList->MapAction(
			GraphEditorCommands::Get().Rename,
			FExecuteAction::CreateLambda([this] {
				if (SelectedNodes.Num() > 0)
				{
					SelectedNodes.Last()->HandleRenameAction();
				}
			})
		);
		UICommandList->MapAction(
			GraphEditorCommands::Get().Copy,
			FExecuteAction::CreateLambda([this] {
				CopySelectedNodes();
			}),
			FCanExecuteAction::CreateLambda([this] {
				return SelectedNodes.Num() > 0;
			})
		);
		UICommandList->MapAction(
			GraphEditorCommands::Get().Paste,
			FExecuteAction::CreateLambda([this] {
				PasteNodes();
			}),
			FCanExecuteAction::CreateLambda([this] {
				return CanPaste();
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
		CurrentTransaction.reset();
		UndoStack.Empty();
		RedoStack.Empty();
		MovingNodes.Empty();
	}

	void SGraphPanel::AddNode(ObjectPtr<GraphNode> NewNodeData, const Vector2D& Pos)
	{
		NewNodeData->SetOuter(GraphData);
		NewNodeData->Position = Pos; 

		if (ShObjectOp* Op = GetShObjectOp(NewNodeData))
		{
			Op->OnSelect(NewNodeData);
		}

		auto NodeWidget = NewNodeData->CreateNodeWidget(this);
		Nodes.Add(NodeWidget);

		ClearSelectedNode();
		GraphData->AddNode(NewNodeData);
		AddSelectedNode(NodeWidget);
	}

	void SGraphPanel::Undo()
	{
		if (!UndoStack.IsEmpty())
		{
			auto State = UndoStack.Pop();
			for (int Index = State.Commands.Num() - 1; Index >= 0; Index--)
			{
				State.Commands[Index]->Undo();
			}
			RedoStack.Add(State);
		}
	}

	void SGraphPanel::Redo()
	{
		if (!RedoStack.IsEmpty())
		{
			auto State = RedoStack.Pop();
			for (const auto& Command : State.Commands)
			{
				Command->Do();
			}
			UndoStack.Add(State);
		}
	}

	void SGraphPanel::AddLink(SGraphPin* Output, SGraphPin* Input)
	{
		GraphData->AddLink(Output->PinData, Input->PinData);
		Links.AddUnique(Output, Input);
	}

	void SGraphPanel::RemoveLink(SGraphPin* Output, SGraphPin* Input)
	{
		Links.Remove(Output, Input);
		GraphData->RemoveLink(Output->PinData, Input->PinData);
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

	SGraphPin* SGraphPanel::GetOuputPin(SGraphPin* InputPin) const
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
				for (auto [OutPinPtr, InPinPtr] : NodeData->OutPinToInPin)
				{
					SGraphPin* OutPin = GetGraphPin(OutPinPtr.GetGuid());
					SGraphPin* InPin = GetGraphPin(InPinPtr.GetGuid());
					if (OutPin && InPin)
					{
						AddLink(OutPin, InPin);
					}
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

			if (ShObjectOp* Op = GetShObjectOp(NodeData))
			{
				Op->OnSelect(NodeData);
			}
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
		if (UICommandList->ProcessCommandBindings(MouseEvent))
		{
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
			if (ShObjectOp* Op = GetShObjectOp(GraphData))
			{
				Op->OnSelect(GraphData);
			}
			if (MarqueeStart)
			{
				if (*MarqueeStart != MarqueeEnd)
				{
					Vector2D MarqueeUpperLeft = { FMath::Min((*MarqueeStart).x, MarqueeEnd.x), FMath::Min((*MarqueeStart).y, MarqueeEnd.y) };
					Vector2D MarqueeBottomRight = { FMath::Max((*MarqueeStart).x, MarqueeEnd.x), FMath::Max((*MarqueeStart).y, MarqueeEnd.y) };
					TArray<TSharedRef<SGraphNode>> SelectedNodeWidgets;
					for (int i = 0; i < Nodes.Num(); i++)
					{
						TSharedRef<SGraphNode> NodeWidget = Nodes[i];
						const FGeometry& NodeGeom = NodeWidget->GetTickSpaceGeometry();
						Vector2D NodeUpperLeft = MyGeometry.AbsoluteToLocal(NodeGeom.GetAbsolutePosition());
						Vector2D NodeBootomRight = MyGeometry.AbsoluteToLocal(NodeGeom.GetAbsolutePosition() + NodeGeom.GetAbsoluteSize());
						if (FSlateRect::DoRectanglesIntersect({ MarqueeUpperLeft, MarqueeBottomRight }, { NodeUpperLeft, NodeBootomRight }))
						{
							SelectedNodeWidgets.Add(NodeWidget);
						}
					}
					for (auto NodeWidget : SelectedNodeWidgets)
					{
						AddSelectedNode(NodeWidget);
					}
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
					Vector2D C0 = GetPinConnectionPoint(OuputPin, MyGeometry, false);
					Vector2D C3 = GetPinConnectionPoint(InputPin, MyGeometry, false);
					double Offset = FMath::Abs(C0.x - C3.x) / 2;
					Vector2D C1 = {C0.x + Offset, C0.y};
					Vector2D C2 = { C3.x - Offset, C3.y };

					if (LineBezierIntersection(*CutLineStart, CutLineEnd, C0, C1, C2, C3))
					{
						ScopedTransaction Transaction(this);
						DoCommand(MakeShared<RemoveLinkCommand>(this, OuputPin->PinData, InputPin->PinData));
					}
				}
				CutLineStart.Reset();
				return FReply::Handled().ReleaseMouseCapture().ReleaseMouseLock();
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
			GraphData->OnDrop(DragDropOp, PanelCoordToGraphCoord(MousePos));
		}
		return FReply::Handled().ReleaseMouseLock();
	}

	FReply SGraphPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList->ProcessCommandBindings(InKeyEvent))
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
		FSlateDrawElement::MakeCubicBezierSpline(OutDrawElements, Layer, PaintGeometry, Start, { Start.x + StartOffset, Start.y }, {End.x + EndOffset, End.y}, End, 2.0f, 
			ESlateDrawEffect::None, FStyleColors::Foreground.GetSpecifiedColor());
	}

	Vector2D SGraphPanel::GetPinConnectionPoint(SGraphPin* Pin, const FGeometry& PanelGeometry, bool bUsePaintSpaceGeometry) const
	{
		if (Pin->Owner->NodeData->IsCollapsed)
		{
			return Pin->Owner->GetCollapsedPinConnectionPoint(PanelGeometry, Pin->PinData->Direction, bUsePaintSpaceGeometry);
		}

		const FGeometry PinGeometry = bUsePaintSpaceGeometry ? Pin->GetPaintSpaceGeometry() : Pin->GetTickSpaceGeometry();
		return PanelGeometry.AbsoluteToLocal(PinGeometry.GetAbsolutePositionAtCoordinates(FVector2D{ 0.5f, 0.5f }));
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
			else if (SelectedNodes.Contains(&*CurNodeWidget))
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					NodeLayer,
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 2, 2 }),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None,
					HasKeyboardFocus() || HasFocusedDescendants() ? FStyleColors::Select.GetSpecifiedColor() : FStyleColors::SelectInactive.GetSpecifiedColor()
				);
			}

			if (CurNodeWidget->NodeData->HasResponse)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					TopNodeLayer,
					CurWidget.Geometry.ToPaintGeometry(),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None,
					FLinearColor{1.0f, 0.55f, 0.0f, 0.4f}
				);
			}

			if (CurNodeWidget->NodeData->GpuTimeMs > 0.0)
			{
				FString TimeText = FString::Printf(TEXT("%.2f ms"), CurNodeWidget->NodeData->GpuTimeMs);
				FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 8);
				const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
				FVector2D TextSize = FontMeasureService->Measure(TimeText, Font);

				FVector2D NodeSize = CurWidget.Geometry.GetLocalSize();
				FVector2D BadgePos{ NodeSize.X - TextSize.X - 3.0, -(TextSize.Y + 3.0) };

				FSlateDrawElement::MakeBox(OutDrawElements, TopNodeLayer + 1,
					CurWidget.Geometry.ToPaintGeometry(
						FVector2D{TextSize.X + 6.0, TextSize.Y + 2.0},
						FSlateLayoutTransform(FVector2D{BadgePos.X - 3.0, BadgePos.Y - 1.0})),
					FAppCommonStyle::Get().GetBrush("WhiteBrush"),
					ESlateDrawEffect::None, FLinearColor{0, 0, 0, 0.6f});

				FSlateDrawElement::MakeText(OutDrawElements, TopNodeLayer + 1,
					CurWidget.Geometry.ToPaintGeometry(BadgePos, TextSize),
					TimeText, Font, ESlateDrawEffect::None, FLinearColor{0.3f, 1.0f, 0.3f});
			}

		}

		if (PreviewStart)
		{
			DrawConnection(AllottedGeometry.ToPaintGeometry(), OutDrawElements, PanelLayer, PreviewStartDir, *PreviewStart, PreviewEnd);
		}

		for (auto [OutPin, InPin] : Links)
		{
			Vector2D OutPos = GetPinConnectionPoint(OutPin, AllottedGeometry, true);
			Vector2D InPos = GetPinConnectionPoint(InPin, AllottedGeometry, true);
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
			Vector2D LowerRight = UpperLeft + MarqueeSize;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				MaxTopNodeLayer,
				AllottedGeometry.ToPaintGeometry(MarqueeSize, FSlateLayoutTransform(UpperLeft)),
				FAppStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				FLinearColor(0.15f, 0.45f, 0.95f, 0.12f)
			);

			TArray<FVector2D> MarqueeOutline;
			MarqueeOutline.Add(UpperLeft);
			MarqueeOutline.Add(Vector2D(LowerRight.x, UpperLeft.y));
			MarqueeOutline.Add(LowerRight);
			MarqueeOutline.Add(Vector2D(UpperLeft.x, LowerRight.y));
			MarqueeOutline.Add(UpperLeft);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				MaxTopNodeLayer + 1,
				AllottedGeometry.ToPaintGeometry(),
				MarqueeOutline,
				ESlateDrawEffect::None,
				FLinearColor(0.15f, 0.55f, 1.0f, 0.95f),
				true,
				1.0f
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

	void SGraphPanel::GenerateMenuNodeItems(const FText& InFilterText)
	{
		MenuNodeItems.Empty();
		if (auto* NodeMetaTypes = RegisteredNodes.Find(GetRegisteredName(GraphData->DynamicMetaType())))
		{
			for (auto NodeMetaType : *NodeMetaTypes)
			{
				FText NodeTitle = static_cast<GraphNode*>(NodeMetaType->GetDefaultObject())->ObjectName;
				if (!InFilterText.IsEmpty())
				{
					if (NodeTitle.ToString().Contains(InFilterText.ToString()))
					{
						MenuNodeItems.Add(MakeShared<FText>(MoveTemp(NodeTitle)));
					}
				}
				else
				{
					MenuNodeItems.Add(MakeShared<FText>(MoveTemp(NodeTitle)));
				}
			}
		}
	}

	TSharedRef<SWidget> SGraphPanel::CreateContextMenu()
	{
		GenerateMenuNodeItems();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSearchBox)
				.HintText(LOCALIZATION("Search"))
				.OnTextChanged_Lambda([this](const FText& InFilterText) {
					GenerateMenuNodeItems(InFilterText);
					MenuNodeList->RequestListRefresh();
				})
				.DelayChangeNotificationsWhileTyping(false)
			]
			+ SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppCommonStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FStyleColors::Dropdown)
				[
					SAssignNew(MenuNodeList, SListView<TSharedPtr<FText>>)
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

		ScopedTransaction Transaction(this);
		ObjectPtr<GraphNode> NewNodeData = NewShObject<GraphNode>(DefaultNodeData->DynamicMetaType(), GetGraphData());
		DoCommand(MakeShared<AddNodeCommand>(this, NewNodeData, PanelCoordToGraphCoord(MousePos)));

		FSlateApplication::Get().DismissAllMenus();
	}

	void SGraphPanel::ClearSelectedNode()
	{
		for (auto It = SelectedNodes.CreateIterator(); It; It++)
		{
			if (ShObjectOp* Op = GetShObjectOp((*It)->NodeData))
			{
				Op->OnCancelSelect((*It)->NodeData);
			}
			It.RemoveCurrent();
		}
	}

	void SGraphPanel::DeleteSelectedNodes()
	{
		for (int Index = SelectedNodes.Num() -1; Index >= 0; Index--)
		{
			SGraphNode* Node = SelectedNodes[Index];
			for (auto [OuputPin, InputPin] : Links)
			{
				if (OuputPin->Owner == Node || InputPin->Owner == Node)
				{
					DoCommand(MakeShared<RemoveLinkCommand>(this, OuputPin->PinData, InputPin->PinData));
				}
			}

			DoCommand(MakeShared<RemoveNodeCommand>(this, Node->NodeData));
		}
	}

	void SGraphPanel::DeleteNode(SGraphNode* Node)
	{
		if (ShObjectOp* Op = GetShObjectOp(Node->NodeData))
		{
			Op->OnCancelSelect(Node->NodeData);
		}
		
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
	}

	SGraphNode* SGraphPanel::GetNode(GraphNode* NodeData)
	{
		for (int i = 0; i < Nodes.Num(); i++)
		{
			if (Nodes[i]->NodeData == NodeData)
			{
				return &*Nodes[i];
			}
		}
		return nullptr;
	}

	void SGraphPanel::CopySelectedNodes()
	{
		if (SelectedNodes.IsEmpty())
		{
			return;
		}

		// Only preserve links between selected nodes. Temporarily strip the
		// OutPinToInPin map and input-pin SourcePin references that point to
		// non-selected nodes, then restore them after serialization.
		TSet<GraphNode*> SelectedNodeSet;
		for (SGraphNode* Node : SelectedNodes)
		{
			SelectedNodeSet.Add(Node->NodeData);
		}

		auto IsPinInSelection = [&](GraphPin* Pin) {
			return Pin && SelectedNodeSet.Contains(Pin->GetOwnerNode());
		};

		TMap<GraphNode*, TMultiMap<ObserverObjectPtr<GraphPin>, ObserverObjectPtr<GraphPin>>> SavedOutMaps;
		TMap<GraphPin*, ObserverObjectPtr<GraphPin>> SavedSourcePins;
		for (SGraphNode* Node : SelectedNodes)
		{
			GraphNode* NodeData = Node->NodeData;
			SavedOutMaps.Add(NodeData, NodeData->OutPinToInPin);
			for (auto It = NodeData->OutPinToInPin.CreateIterator(); It; ++It)
			{
				if (!IsPinInSelection(It.Value().Get()))
				{
					It.RemoveCurrent();
				}
			}
			for (auto& Pin : NodeData->Pins)
			{
				if (Pin->SourcePin.IsValid() && !IsPinInSelection(Pin->SourcePin.Get()))
				{
					SavedSourcePins.Add(Pin.Get(), Pin->SourcePin);
					Pin->SourcePin.Reset();
				}
			}
		}

		ClipboardData.Reset();
		FMemoryWriter Ar(ClipboardData);
		int32 Num = SelectedNodes.Num();
		Ar << Num;
		for (SGraphNode* Node : SelectedNodes)
		{
			FString TypeName = GetRegisteredName(Node->NodeData->DynamicMetaType());
			Ar << TypeName;
			Node->NodeData->Serialize(Ar);
		}

		// Restore.
		for (auto& [NodeData, SavedMap] : SavedOutMaps)
		{
			NodeData->OutPinToInPin = MoveTemp(SavedMap);
		}
		for (auto& [Pin, SavedSrc] : SavedSourcePins)
		{
			Pin->SourcePin = SavedSrc;
		}
	}

	void SGraphPanel::PasteNodes()
	{
		if (!CanPaste())
		{
			return;
		}

		FMemoryReader Ar(ClipboardData);
		int32 Num = 0;
		Ar << Num;

		TArray<ObjectPtr<GraphNode>> NewNodes;
		BeginObjectPtrFixup();
		// Pre-register all currently alive ShObjects in the fixup map so that
		// external references can still resolve.
		for (ShObject* LiveObject : GlobalValidShObjects)
		{
			RegisterLoadedShObject(LiveObject);
		}
		for (int32 i = 0; i < Num; i++)
		{
			FString TypeName;
			Ar << TypeName;
			MetaType* Mt = GetMetaType(TypeName);
			auto NewNode = NewShObject<GraphNode>(Mt, GraphData);
			NewNode->Serialize(Ar);
			NewNodes.Add(MoveTemp(NewNode));
		}
		ResolveObjectPtrFixups();
		EndObjectPtrFixup();

		// Collect intra-selection link pairs and clear link state on clones
		// (AddLinkCommand will re-populate it). 
		struct LinkPair { GraphPin* Out; GraphPin* In; };
		TArray<LinkPair> LinkPairs;
		for (auto& NewNode : NewNodes)
		{
			for (auto& [OutPtr, InPtr] : NewNode->OutPinToInPin)
			{
				if (OutPtr.Get() && InPtr.Get())
				{
					LinkPairs.Add({ OutPtr.Get(), InPtr.Get() });
				}
			}
			NewNode->OutPinToInPin.Reset();
			for (auto& Pin : NewNode->Pins)
			{
				Pin->SourcePin.Reset();
			}
		}

		// Offset positions so the first pasted node lands at the mouse cursor.
		const Vector2D MouseGraph = PanelCoordToGraphCoord(MousePos);
		const Vector2D Offset = MouseGraph - NewNodes[0]->Position;

		// Regenerate Guids on cloned nodes and all their subobjects so they don't collide
		// with the originals.
		for (auto& NewNode : NewNodes)
		{
			NewNode->RegenerateGuidRecursive();
			NewNode->Position += Offset;
		}

		ScopedTransaction Transaction(this);
		for (auto& NewNode : NewNodes)
		{
			DoCommand(MakeShared<AddNodeCommand>(this, NewNode, NewNode->Position));
		}
		for (const LinkPair& Pair : LinkPairs)
		{
			DoCommand(MakeShared<AddLinkCommand>(this, Pair.Out, Pair.In));
		}

		// Select all pasted nodes.
		ClearSelectedNode();
		TArray<TSharedRef<SGraphNode>> NewWidgets;
		for (int32 i = 0; i < Nodes.Num(); i++)
		{
			if (NewNodes.Contains(Nodes[i]->NodeData))
			{
				NewWidgets.Add(Nodes[i]);
			}
		}
		for (auto& Widget : NewWidgets)
		{
			AddSelectedNode(Widget);
		}
	}

	void RenameNodeCommand::Do()
	{
		NodeData->ObjectName = NewName;
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void RenameNodeCommand::Undo()
	{
		NodeData->ObjectName = OldName;
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void AddNodeCommand::Do()
	{
		GraphPanel->AddNode(NodeData, Pos);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void AddNodeCommand::Undo()
	{
		GraphPanel->DeleteNode(GraphPanel->GetNode(NodeData));
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void RemoveNodeCommand::Do()
	{
		GraphPanel->DeleteNode(GraphPanel->GetNode(NodeData));
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void RemoveNodeCommand::Undo()
	{
		GraphPanel->AddNode(NodeData, NodeData->Position);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void AddLinkCommand::Do()
	{
		SGraphPin* OutputPin = GraphPanel->GetGraphPin(Output->GetGuid());
		SGraphPin* InputPin = GraphPanel->GetGraphPin(Input->GetGuid());
		GraphPanel->AddLink(OutputPin, InputPin);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void AddLinkCommand::Undo()
	{
		SGraphPin* OutputPin = GraphPanel->GetGraphPin(Output->GetGuid());
		SGraphPin* InputPin = GraphPanel->GetGraphPin(Input->GetGuid());
		GraphPanel->RemoveLink(OutputPin, InputPin);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void RemoveLinkCommand::Do()
	{
		SGraphPin* OutputPin = GraphPanel->GetGraphPin(Output->GetGuid());
		SGraphPin* InputPin = GraphPanel->GetGraphPin(Input->GetGuid());
		GraphPanel->RemoveLink(OutputPin, InputPin);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void RemoveLinkCommand::Undo()
	{
		SGraphPin* OutputPin = GraphPanel->GetGraphPin(Output->GetGuid());
		SGraphPin* InputPin = GraphPanel->GetGraphPin(Input->GetGuid());
		GraphPanel->AddLink(OutputPin, InputPin);
		GraphPanel->GetGraphData()->MarkDirty();
	}

	void MoveNodeCommand::Do()
	{
		NodeData->Position = NewPos;
		Vector2D DeltaPos = NewPos - OldPos;
		if (FMath::Abs(DeltaPos.x) > 0.8 || FMath::Abs(DeltaPos.y) > 0.8)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

	void MoveNodeCommand::Undo()
	{
		NodeData->Position = OldPos;
		Vector2D DeltaPos = NewPos - OldPos;
		if (FMath::Abs(DeltaPos.x) > 0.8 || FMath::Abs(DeltaPos.y) > 0.8)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

	void ResizeNodeCommand::Do()
	{
		NodeData->NodeWidth = NewWidth;
		if (FMath::Abs(NewWidth - OldWidth) > 0.8f)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

	void ResizeNodeCommand::Undo()
	{
		NodeData->NodeWidth = OldWidth;
		if (FMath::Abs(NewWidth - OldWidth) > 0.8f)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

	void SetNodeCollapsedCommand::Do()
	{
		NodeData->IsCollapsed = NewCollapsed;
		if (OldCollapsed != NewCollapsed)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

	void SetNodeCollapsedCommand::Undo()
	{
		NodeData->IsCollapsed = OldCollapsed;
		if (OldCollapsed != NewCollapsed)
		{
			GraphPanel->GetGraphData()->MarkDirty();
		}
	}

}


