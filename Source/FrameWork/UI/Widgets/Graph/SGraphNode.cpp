#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "Styling/StyleColors.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Graph/SGraphPin.h"
#include <Framework/Commands/GenericCommands.h>

namespace FW
{
	
	void SGraphNode::Construct(const FArguments& InArgs, SGraphPanel* InOwnerPanel)
	{
		NodeData = InArgs._NodeData;
		Owner = InOwnerPanel;

		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			FGenericCommands::Get().Delete,
			FExecuteAction::CreateRaw(this, &SGraphNode::OnHandleDeleteAction)
		);
		UICommandList->MapAction(
			FGenericCommands::Get().Rename,
			FExecuteAction::CreateRaw(this, &SGraphNode::OnHandleRenameAction)
		);

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
					.Text_Lambda([this] { return NodeData->NodeTitle; })
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
						NodeData->NodeTitle = NewText;
					})
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppCommonStyle::Get().GetBrush("Graph.NodeContentBackground"))
				.Padding(FMargin{ 0.0f, 4.0f, 0.0f, 0.0f })
				[
					PinContainer
				]
			]
		];

		for (GraphPin* Pin : NodeData->GetPins())
		{
			auto PinIcon = SNew(SGraphPin, this).PinData(Pin);
			Pins.Add(&*PinIcon);
			auto PinDesc = SNew(SBox).MinDesiredWidth(100.0f)
				[
					SNew(STextBlock)
						.Text(Pin->PinName)
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
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
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
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
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
				Owner->ClearSelectedNode();
				Owner->AddSelectedNode(SharedThis(this));
			}
			MousePos = MouseEvent.GetScreenSpacePosition();
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
				Owner->ClearSelectedNode();
				Owner->AddSelectedNode(SharedThis(this));
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	TSharedRef<SWidget> SGraphNode::CreateContextMenu()
	{
		FMenuBuilder MenuBuilder{ true, UICommandList };
		MenuBuilder.BeginSection("Control", FText::FromString("Control"));
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	void SGraphNode::OnHandleDeleteAction()
	{
		Owner->DeleteSelectedNodes();
	}

	void SGraphNode::OnHandleRenameAction()
	{
		NodeTitleEditText->EnterEditingMode();
	}

	FReply SGraphNode::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
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
