#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "Styling/StyleColors.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Graph/SGraphPin.h"
#include "Editor/GraphEditorCommands.h"

namespace FW
{
	
	void SGraphNode::Construct(const FArguments& InArgs, SGraphPanel* InOwnerPanel)
	{
		NodeData = InArgs._NodeData;
		Owner = InOwnerPanel;

		TSharedRef<SVerticalBox> PinContainer = SNew(SVerticalBox);

		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppCommonStyle::Get().GetBrush("Graph.NodeTitleBackground"))
				.BorderBackgroundColor(NodeData->GetNodeColor())
				.HAlign(HAlign_Center)
				[
					SAssignNew(NodeTitleEditText, SInlineEditableTextBlock)
                    .IsSelected_Lambda([] {return false; })
					.Text_Lambda([this] { return NodeData->ObjectName; })
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
						SGraphPanel::ScopedTransaction Transaction{ Owner };
						Owner->DoCommand(MakeShared<RenameNodeCommand>(Owner, NodeData, NodeData->ObjectName, NewText));
					})
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppCommonStyle::Get().GetBrush("Graph.NodeContentBackground"))
				.Padding(FMargin{ 0.0f, 3.0f, 0.0f, 0.0f })
				[
					PinContainer
				]
			]
		];

		for (GraphPin* Pin : NodeData->Pins)
		{
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

		if (TSharedPtr<SWidget> ExtraWidget = NodeData->ExtraNodeWidget())
		{
			PinContainer->AddSlot()[ExtraWidget.ToSharedRef()];
		}
	}

	FReply SGraphNode::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
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
            ShObjectOp* Op = GetShObjectOp(NodeData);
            Op->OnSelect(NodeData);

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
		if (OutDegreeDeps.Contains(InNode))
		{
			OutDegreeDeps[InNode]++;
		}
		else
		{
			OutDegreeDeps.Add(InNode);
			Owner->GetGraphData()->AddDep( InNode->NodeData, this->NodeData);
		}
	}

	void SGraphNode::RemoveDep(SGraphNode* InNode)
	{
		if (--OutDegreeDeps[InNode] <= 0)
		{
			OutDegreeDeps.Remove(InNode);
			Owner->GetGraphData()->RemoveDep(InNode->NodeData, this->NodeData);
		}
	}

}
