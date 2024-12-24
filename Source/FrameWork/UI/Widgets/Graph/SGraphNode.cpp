#include "CommonHeader.h"
#include "SGraphNode.h"
#include "AssetObject/Graph.h"
#include "Styling/StyleColors.h"
#include "SGraphPanel.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK
{
	
	void SGraphNode::Construct(const FArguments& InArgs)
	{
		NodeData = InArgs._NodeData;

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
				.Padding(FMargin{ 0.0f, 5.0f, 0.0f, 0.0f })
				[
					PinContainer
				]
			]
		];

		for (GraphPin* Pin : NodeData->GetPins())
		{
			auto PinContent = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.ColorAndOpacity(Pin->PinColor)
						.DesiredSizeOverride(FVector2D{13,13})
						.Image(FAppStyle::Get().GetBrush("Icons.BulletPoint"))
				]
				+ SHorizontalBox::Slot()
				.Padding(5.0f, 0.0f, 0.0f, 0.0f)
				.HAlign(HAlign_Left)
				[
					SNew(SBox).MinDesiredWidth(100.0f)
					[
						SNew(STextBlock)
							.Text(Pin->PinName)
					]
					
				];

			if (Pin->Direction == PinDirection::Input)
			{
				PinContainer->AddSlot().HAlign(HAlign_Left)
				.Padding(0.0f, 0.0f, 0.0f, 5.0f)
				[
					PinContent
				];
			}
			else
			{
				PinContainer->AddSlot().HAlign(HAlign_Right)
				.Padding(0.0f, 0.0f, 0.0f, 5.0f)
				[
					PinContent
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
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			Owner->SetSelectedNode(this);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
}
