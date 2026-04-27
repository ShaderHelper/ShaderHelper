#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "Styling/StyleColors.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Graph/SGraphPin.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "Editor/GraphEditorCommands.h"

namespace FW
{
	namespace
	{
		constexpr float NodeResizeHandleWidth = 6.0f;
	}

	
	void SGraphNode::Construct(const FArguments& InArgs, SGraphPanel* InOwnerPanel)
	{
		NodeData = InArgs._NodeData;
		Owner = InOwnerPanel;

		PinContainer = SNew(SVerticalBox);

		ChildSlot
		[
			SNew(SBox)
			.WidthOverride_Lambda([this] {
				return NodeData->NodeWidth;
			})
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(NodeTitleWidget, SBorder)
					.BorderImage(FAppCommonStyle::Get().GetBrush("Graph.NodeTitleBackground"))
					.BorderBackgroundColor(NodeData->GetNodeColor())
					.HAlign(HAlign_Fill)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SIconButton)
							.ButtonStyle(&FAppCommonStyle::Get().GetWidgetStyle<FButtonStyle>("SuperSimpleButton"))
							.Icon_Lambda([this] {
								return NodeData->IsCollapsed ? FAppStyle::Get().GetBrush("Icons.ChevronRight") : FAppStyle::Get().GetBrush("Icons.ChevronDown");
							})
							.IconSize(FVector2D{ 12.0f, 12.0f })
							.IsFocusable(false)
							.OnClicked_Lambda([this] {
								SGraphPanel::ScopedTransaction Transaction{ Owner };
								Owner->DoCommand(MakeShared<SetNodeCollapsedCommand>(Owner, NodeData, NodeData->IsCollapsed, !NodeData->IsCollapsed));
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SAssignNew(NodeTitleEditText, SInlineEditableTextBlock)
							.IsSelected_Lambda([] {return false; })
							.Text_Lambda([this] { return NodeData->ObjectName; })
							.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
								SGraphPanel::ScopedTransaction Transaction{ Owner };
								Owner->DoCommand(MakeShared<RenameNodeCommand>(Owner, NodeData, NodeData->ObjectName, NewText));
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SSpacer).Size(FVector2D{ 12.0f, 1.0f })
						]
					]
				]
				+SVerticalBox::Slot()
				[
					SNew(SBorder)
					.BorderImage(FAppCommonStyle::Get().GetBrush("Graph.NodeContentBackground"))
					.Padding(FMargin{ 0.0f, 3.0f, 0.0f, 0.0f })
					.Visibility_Lambda([this] {
						return NodeData->IsCollapsed ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						PinContainer.ToSharedRef()
					]
				]
			]
		];

		PopulatePinWidgets();
	}

	void SGraphNode::PopulatePinWidgets()
	{
		Pins.Empty();
		PinContainer->ClearChildren();

		for (GraphPin* Pin : NodeData->Pins)
		{
			if (!Pin->ShouldLayoutInNode())
			{
				continue;
			}
			auto PinIcon = SNew(SGraphPin, this).PinData(Pin);
			Pins.Add(&*PinIcon);
			auto PinDesc = SNew(SBox).MinDesiredWidth(100.0f)
				[
					SNew(STextBlock)
						.Text(Pin->ObjectName)
				];

			auto InputPinContent = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PinIcon
				]
				+ SHorizontalBox::Slot()
				.Padding(2.0f, 0.0f, 0.0f, 0.0f)
				.AutoWidth()
				[
					PinDesc
				];

			auto OutputPinContent = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 2.0f, 0.0f)
				.AutoWidth()
				[
					PinDesc
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PinIcon
				];

			if (Pin->Direction == PinDirection::Input)
			{
				PinDesc->SetHAlign(HAlign_Left);
				PinContainer->AddSlot()
				.HAlign(HAlign_Left)
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 3.0f)
				[
					InputPinContent
				];
			}
			else
			{
				PinDesc->SetHAlign(HAlign_Right);
				PinContainer->AddSlot()
				.HAlign(HAlign_Right)
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 3.0f)
				[
					OutputPinContent
				];
			}
		}

		if (TSharedPtr<SWidget> ExtraWidget = NodeData->ExtraNodeWidget(this))
		{
			PinContainer->AddSlot()[ExtraWidget.ToSharedRef()];
		}
	}

	void SGraphNode::RebuildPins()
	{
		// Remove all links involving this node's pins
		for (SGraphPin* Pin : Pins)
		{
			if (Pin->PinData->Direction == PinDirection::Input)
			{
				if (SGraphPin* OutputPin = Owner->GetOuputPin(Pin))
				{
					Owner->Links.Remove(OutputPin, Pin);
				}
			}
			else
			{
				Owner->Links.Remove(Pin);
			}
		}

		PopulatePinWidgets();

		// Restore links from data model state
		for (SGraphPin* Pin : Pins)
		{
			if (Pin->PinData->Direction == PinDirection::Input && Pin->PinData->SourcePin.IsValid())
			{
				if (SGraphPin* OutputPin = Owner->GetGraphPin(Pin->PinData->SourcePin.GetGuid()))
				{
					Owner->Links.AddUnique(OutputPin, Pin);
				}
			}
			else if (Pin->PinData->Direction == PinDirection::Output)
			{
				TArray<ObserverObjectPtr<GraphPin>> LinkedInputPins;
				NodeData->OutPinToInPin.MultiFind(Pin->PinData, LinkedInputPins);
				for (const ObserverObjectPtr<GraphPin>& InputPinPtr : LinkedInputPins)
				{
					if (SGraphPin* InputPin = Owner->GetGraphPin(InputPinPtr.GetGuid()))
					{
						Owner->Links.AddUnique(Pin, InputPin);
					}
				}
			}
		}
	}

	FReply SGraphNode::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsOnResizeHandle(MyGeometry, MouseEvent.GetScreenSpacePosition()))
		{
			if (!Owner->IsSelectedNode(this))
			{
				if (!MouseEvent.IsShiftDown())
				{
					Owner->ClearSelectedNode();
				}

				Owner->AddSelectedNode(SharedThis(this));
			}
			if (ShObjectOp* Op = GetShObjectOp(NodeData))
			{
				Op->OnSelect(NodeData);
			}

			const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			bIsResizingWidth = true;
			ResizeStartWidth = NodeData->NodeWidth;
			ResizeStartMouseX = (float)LocalMousePos.X;
			return FReply::Handled().CaptureMouse(AsShared());
		}

		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			if (!Owner->IsSelectedNode(this))
			{
				if (!MouseEvent.IsShiftDown())
				{
					Owner->ClearSelectedNode();
				}
				
				Owner->AddSelectedNode(SharedThis(this));
			}
			MousePos = MouseEvent.GetScreenSpacePosition();
			if (ShObjectOp* Op = GetShObjectOp(NodeData))
			{
				Op->OnSelect(NodeData);
			}

			if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				for (auto Node : Owner->SelectedNodes)
				{
					Owner->MovingNodes.Emplace( Node->NodeData, Node->NodeData->Position );
				}
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphNode::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (bIsResizingWidth && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			bIsResizingWidth = false;
			const float NewWidth = NodeData->NodeWidth;
			if (FMath::Abs(NewWidth - ResizeStartWidth) > 0.8f)
			{
				SGraphPanel::ScopedTransaction Transaction{ Owner };
				Owner->DoCommand(MakeShared<ResizeNodeCommand>(Owner, NodeData, ResizeStartWidth, NewWidth));
			}
			return FReply::Handled().ReleaseMouseCapture();
		}

		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, CreateContextMenu(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect::ContextMenu);
			return FReply::Handled();
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			FVector2D Offset = MouseEvent.GetScreenSpacePosition() - MousePos;
			if (Owner->IsMultiSelect() && Offset.Equals(FVector2D::Zero(), 0.01))
			{
				if (!MouseEvent.IsShiftDown())
				{
					Owner->ClearSelectedNode();
				}
				
				Owner->AddSelectedNode(SharedThis(this));
			}

			SGraphPanel::ScopedTransaction Transaction{ Owner };
			for (auto [NodeData, OldPos] : Owner->MovingNodes)
			{
				Owner->DoCommand(MakeShared<MoveNodeCommand>(Owner, NodeData, OldPos, NodeData->Position));
			}
			Owner->MovingNodes.Empty();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	FReply SGraphNode::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (bIsResizingWidth)
		{
			const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			NodeData->NodeWidth = FMath::Max(GraphNode::MinNodeWidth, ResizeStartWidth + LocalMousePos.X - ResizeStartMouseX);
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FCursorReply SGraphNode::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
	{
		return bIsResizingWidth || IsOnResizeHandle(MyGeometry, CursorEvent.GetScreenSpacePosition())
			? FCursorReply::Cursor(EMouseCursor::ResizeLeftRight)
			: FCursorReply::Unhandled();
	}

	bool SGraphNode::IsOnResizeHandle(const FGeometry& MyGeometry, const FVector2D& ScreenSpacePosition) const
	{
		const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(ScreenSpacePosition);
		const float NodeWidth = MyGeometry.GetLocalSize().X;
		return LocalMousePos.X >= NodeWidth - NodeResizeHandleWidth && LocalMousePos.X <= NodeWidth + NodeResizeHandleWidth;
	}

	Vector2D SGraphNode::GetCollapsedPinConnectionPoint(const FGeometry& PanelGeometry, PinDirection Direction, bool bUsePaintSpaceGeometry) const
	{
		FGeometry AnchorGeometry = bUsePaintSpaceGeometry ? GetPaintSpaceGeometry() : GetTickSpaceGeometry();
		if (NodeTitleWidget.IsValid())
		{
			AnchorGeometry = bUsePaintSpaceGeometry ? NodeTitleWidget->GetPaintSpaceGeometry() : NodeTitleWidget->GetTickSpaceGeometry();
		}

		const FVector2D AnchorCoordinates = Direction == PinDirection::Input ? FVector2D{ 0.0f, 0.5f } : FVector2D{ 1.0f, 0.5f };
		return PanelGeometry.AbsoluteToLocal(AnchorGeometry.GetAbsolutePositionAtCoordinates(AnchorCoordinates));
	}

	TSharedRef<SWidget> SGraphNode::CreateContextMenu()
	{
		FMenuBuilder MenuBuilder{ true, Owner->UICommandList };
		MenuBuilder.BeginSection("Control", FText::FromString("Control"));
		{
			MenuBuilder.AddMenuEntry(GraphEditorCommands::Get().Delete, NAME_None, {}, {}, 
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Delete" });
			MenuBuilder.AddMenuEntry(GraphEditorCommands::Get().Rename, NAME_None, {}, {},
				FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Rename" });
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	void SGraphNode::HandleRenameAction()
	{
		NodeTitleEditText->EnterEditingMode();
	}

	void SGraphNode::AddDep(SGraphNode* InNode)
	{
		Owner->GetGraphData()->AddDep(InNode->NodeData, this->NodeData);
	}

	void SGraphNode::RemoveDep(SGraphNode* InNode)
	{
		Owner->GetGraphData()->RemoveDep(InNode->NodeData, this->NodeData);
	}

}
