#include "CommonHeader.h"
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{

	void SPropertyCatergory::Construct(const FArguments& InArgs, const TSharedPtr<class ITableRow>& TableRow)
	{
		OwnerRowPtr = TableRow.Get();

		const FSlateBrush* CategoryColor = nullptr;
		FSlateFontInfo CategoryTextFont;

		if (InArgs._IsRootCategory)
		{
			CategoryColor = FAppCommonStyle::Get().GetBrush("PropertyView.CategoryColor");
			CategoryTextFont = FAppStyle::Get().GetFontStyle("SmallFontBold");
		}
		else
		{
			CategoryColor = FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
			CategoryTextFont = FAppStyle::Get().GetFontStyle("SmallFont");
		}

		TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, TableRow)
				.IndentAmount(0)
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InArgs._DisplayName))
				.Font(MoveTemp(CategoryTextFont))
				.ColorAndOpacity(FSlateColor{ FLinearColor{0.8f,0.8f,0.8f} })
				
			];

		if (InArgs._AddMenuWidget)
		{
			HBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.HasDownArrow(false)
				.MenuContent()
				[
					InArgs._AddMenuWidget.ToSharedRef()
				]
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
		}

		ChildSlot
		[
			SNew(SBorder)
			.Padding(FMargin{0.0f, 3.0f, 0.0f, 0.0f})
			[
				SNew(SBorder)
				.BorderImage(CategoryColor)
				[
					HBox
				]
				
			]
		
		];
	}


	FReply SPropertyCatergory::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			OwnerRowPtr->ToggleExpansion();
		}

		return FReply::Handled();
	}
}
