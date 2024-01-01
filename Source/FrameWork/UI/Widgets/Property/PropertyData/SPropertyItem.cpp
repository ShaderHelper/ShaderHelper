#include "CommonHeader.h"
#include "SPropertyItem.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace FRAMEWORK
{

	void SPropertyItem::Construct(const FArguments& InArgs)
	{
		DisplayNameText = FText::FromString(InArgs._DisplayName);
		OnDisplayNameChanged = InArgs._OnDisplayNameChanged;
		TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.DesiredSizeOverride(FVector2D(10.0f, 0.0f))
				.Image(FAppCommonStyle::Get().GetBrush("PropertyView.RowIndentDropShadow"))
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 14.0f, 0.0f)
			.FillWidth(1)
			[
				SNew(SInlineEditableTextBlock)
				.IsSelected_Lambda([] {return true; })
				.Text(this, &SPropertyItem::GetNameText)
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
					DisplayNameText = NewText;
					if (OnDisplayNameChanged)
					{
						OnDisplayNameChanged(NewText.ToString());
					}
				})
				.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor{ FLinearColor{0.8f,0.8f,0.8f} })
			]
			+ SHorizontalBox::Slot()
			.FillWidth(3)
			[
				InArgs._ValueWidget.ToSharedRef()
			];

		HBox->SetEnabled(InArgs._IsEnabled);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(FMargin{ 0.0f, 3.0f, 0.0f, 0.0f })
			[
				SNew(SBorder)
				.BorderImage_Lambda([this] {
					if (this->IsHovered())
					{
						return FAppCommonStyle::Get().GetBrush("PropertyView.ItemHoverdColor");
					}
					return FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
				})
				[
					HBox
				]

			]
		];
	}

}
