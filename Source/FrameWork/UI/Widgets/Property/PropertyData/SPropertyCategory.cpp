#include "CommonHeader.h"
#include "SPropertyCategory.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{

	void SPropertyCatergory::Construct(const FArguments& InArgs, const TSharedPtr<class ITableRow>& TableRow)
	{
		OwnerRowPtr = TableRow.Get();
		bArrayElementStyle = InArgs._ArrayElementStyle;

		const FSlateBrush* NoBrush = FAppStyle::Get().GetBrush("NoBrush");
		const FSlateBrush* OuterBrush = bArrayElementStyle ? FAppStyle::Get().GetBrush("Brushes.Input") : FAppStyle::Get().GetBrush("Brushes.Recessed");
		const FMargin OuterPadding = bArrayElementStyle ? FMargin{3.0f, 1.0f, 3.0f, 0.0f} : FMargin{0.0f, 3.0f, 0.0f, 0.0f};
		const FSlateBrush* CategoryBrush = nullptr;
		FSlateFontInfo CategoryTextFont;

		if (InArgs._IsRootCategory)
		{
			CategoryBrush = InArgs._CategoryBrush ? InArgs._CategoryBrush : FAppCommonStyle::Get().GetBrush("PropertyView.CategoryColor");
			CategoryTextFont = FAppStyle::Get().GetFontStyle("NormalFontBold");
		}
		else
		{
			CategoryBrush = InArgs._CategoryBrush ? InArgs._CategoryBrush : FAppCommonStyle::Get().GetBrush("PropertyView.ItemColor");
			CategoryTextFont = FAppStyle::Get().GetFontStyle("NormalFont");
		}

		TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, TableRow)
				.IndentAmount(0)
			];

		if (InArgs._HasCheckBox)
		{
			HBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SNew(SCheckBox)
				.IsChecked(InArgs._CheckBoxState)
				.OnCheckStateChanged(InArgs._OnCheckBoxStateChanged)
			];
		}

		HBox->AddSlot()
		.VAlign(VAlign_Center)
		.Padding(4.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(InArgs._DisplayName)
			.Font(MoveTemp(CategoryTextFont))
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
			.BorderImage_Lambda([this, OuterBrush, NoBrush] {
				return bArrayElementStyle && OwnerRowPtr && OwnerRowPtr->IsItemSelected() ? NoBrush : OuterBrush;
			})
			.Padding(OuterPadding)
			[
				SNew(SBorder)
				.BorderImage_Lambda([this, CategoryBrush, NoBrush] {
					return bArrayElementStyle && OwnerRowPtr && OwnerRowPtr->IsItemSelected() ? NoBrush : CategoryBrush;
				})
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
			if (!bArrayElementStyle)
			{
				OwnerRowPtr->ToggleExpansion();
				return FReply::Handled();
			}
		}

		return FReply::Unhandled();
	}
}
