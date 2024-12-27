#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "Styling/StyleColors.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Framework/Commands/GenericCommands.h>

namespace FRAMEWORK
{
	
	void SGraphNode::Construct(const FArguments& InArgs)
	{
		NodeData = InArgs._NodeData;

		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			FGenericCommands::Get().Delete,
			FExecuteAction::CreateRaw(this, &SGraphNode::OnHandleDeleteAction)
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
					SNew(STextBlock)
					.Text(NodeData->GetNodeTitle())
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
			auto PinIcon = SNew(SImage)
				.ColorAndOpacity(Pin->GetPinColor())
				.DesiredSizeOverride(FVector2D{ 13,13 })
				.Image(FAppStyle::Get().GetBrush("Icons.BulletPoint"));
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
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.AutoWidth()
				[
					PinDesc
				];

			auto OutputPinContent = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				.AutoWidth()
				[
					PinDesc
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					PinIcon
				];

			if (Pin->Direction == PinDirection::Input)
			{
				PinDesc->SetHAlign(HAlign_Left);

				PinContainer->AddSlot()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					InputPinContent
				];
			}
			else
			{
				PinDesc->SetHAlign(HAlign_Right);

				PinContainer->AddSlot()
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
			Owner->SetSelectedNode(StaticCastSharedRef<SGraphNode>(AsShared()));
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
		return FReply::Unhandled();
	}

	TSharedRef<SWidget> SGraphNode::CreateContextMenu()
	{
		FMenuBuilder MenuBuilder{ true, UICommandList };
		MenuBuilder.BeginSection("Control", FText::FromString("Control"));
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	void SGraphNode::OnHandleDeleteAction()
	{
		Owner->DeleteNode(StaticCastSharedRef<SGraphNode>(AsShared()));
	}

	FReply SGraphNode::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList.IsValid() && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

}
