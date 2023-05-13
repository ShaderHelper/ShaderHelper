#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/Styles/FShaderHelperStyle.h"

namespace SH
{

	void SShaderEditorBox::Construct(const FArguments& InArgs)
	{
		TSharedPtr<SScrollBar> VScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Vertical);
		TSharedPtr<SScrollBar> HScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Horizontal);

		Marshaller = MakeShared<FShaderEditorMarshaller>();

		CurLineNum = 1;
		LineNumberData = { MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))) };
		CurCursorLineIndex = LastCursorLineIndex = -1;

		ChildSlot
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Black)
			[
				SNew(SGridPanel)
				.FillColumn(0, 1.0f)
				.FillRow(0, 1.0f)
				+ SGridPanel::Slot(0, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(LineNumberList, SListView<LineNumberItemPtr>)
						.ListItemsSource(&LineNumberData)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SShaderEditorBox::GenerateRowForItem)
						.ScrollbarVisibility(EVisibility::Collapsed)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSpacer)
						.Size(FVector2D(10, 10))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(0.95f)
					[
						SNew(SMultiLineEditableText)
						.Text(InArgs._Text)
						.Marshaller(Marshaller)
						.OnCursorMoved(this, &SShaderEditorBox::OnCursorMoved)
						.OnTextChanged(this, &SShaderEditorBox::OnShaderTextChanged)
						.VScrollBar(VScrollBar)
						.HScrollBar(HScrollBar)
					]
			
				]
				+ SGridPanel::Slot(1, 0)
				[
					VScrollBar.ToSharedRef()
				]
				+ SGridPanel::Slot(0, 1)
				[
					HScrollBar.ToSharedRef()
				]
			]
				
		];
	}

	void SShaderEditorBox::OnShaderTextChanged(const FText& InText)
	{
		const TArray< FTextLayout::FLineView >& LineViews = Marshaller->TextLayout->GetLineViews();
		const int32 LineViewNum = LineViews.Num();
		int32 DiffNum = FMath::Abs(LineViewNum - CurLineNum);
		if (LineViewNum >= CurLineNum)
		{
			while (DiffNum--)
			{
				CurLineNum++;
				LineNumberData.Add(MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))));
			}
		}
		else
		{
			while (DiffNum--)
			{
				CurLineNum--;
				LineNumberData.Pop();
			}
		}
		
		LineNumberList->RequestListRefresh();
	}

	const FLinearColor LineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };

	void SShaderEditorBox::OnCursorMoved(const FTextLocation& InTextLocaction)
	{
		LastCursorLineIndex = CurCursorLineIndex;
		CurCursorLineIndex = InTextLocaction.GetLineIndex();
		if (CurCursorLineIndex < LineNumberSlateTexts.Num())
		{
			LineNumberSlateTexts[CurCursorLineIndex]->SetColorAndOpacity(FLinearColor::White);
			if (LastCursorLineIndex != CurCursorLineIndex && LastCursorLineIndex >= 0)
			{
				LineNumberSlateTexts[LastCursorLineIndex]->SetColorAndOpacity(LineNumberTextColor);
			}
		}
	}

	TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
			.ColorAndOpacity(LineNumberTextColor)
			.Text(*Item)
			.Justification(ETextJustify::Right)
			.MinDesiredWidth(15.0f);

		if (CurCursorLineIndex == LineNumberSlateTexts.Num())
		{
			LineNumberTextBlock->SetColorAndOpacity(FLinearColor::White);
			if (LastCursorLineIndex != CurCursorLineIndex && LastCursorLineIndex >= 0)
			{
				LineNumberSlateTexts[LastCursorLineIndex]->SetColorAndOpacity(LineNumberTextColor);
			}
		}
		
		LineNumberSlateTexts.Add(LineNumberTextBlock);
		return SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
			.Content()
			[
				LineNumberTextBlock.ToSharedRef()
			];
	}

	void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;
	}

	void FShaderEditorMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

}