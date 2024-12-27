#include "CommonHeader.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Misc/CommonCommands.h"
#include <Widgets/Input/SSearchBox.h>
#include <Styling/StyleColors.h>
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
		auto NodeWidget = InNodeData->CreateNodeWidget();
		NodeWidget->SetOwner(this);
		Nodes.Add(NodeWidget);
		return NodeWidget;
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

	FReply SGraphPanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (GraphData)
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
		const Vector2D CursorDelta = MouseEvent.GetCursorDelta();

		Vector2D CurMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Vector2D DeltaPos = CurMousePos - MousePos;
		MousePos = CurMousePos;

		if (bIsLeftMouseButtonDown)
		{
			if (SelectedNode.IsValid())
			{
				SelectedNode.Pin()->NodeData->Position += DeltaPos;
				GraphData->MarkDirty();
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

	FReply SGraphPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
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
					CurWidget.Geometry.ToInflatedPaintGeometry(FVector2D{ 1.2,1.2}),
					FAppCommonStyle::Get().GetBrush("Graph.NodeOutline"),
					ESlateDrawEffect::None
				);
			}
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

	void SGraphPanel::DeleteNode(TSharedRef<SGraphNode> InNode)
	{
		GraphData->RemoveNode(InNode->NodeData->Guid);
		Nodes.Remove(MoveTemp(InNode));

		GraphData->MarkDirty();
	}

}


