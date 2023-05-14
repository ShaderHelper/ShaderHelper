#include "CommonHeader.h"
#include "SShaderEditorBox.h"
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
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 5))
					]

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
						SAssignNew(ShaderMultiLineEditableText, SMultiLineEditableText)
						.Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
						.Text(this, &SShaderEditorBox::GetShadedrCode)
						.Marshaller(Marshaller)
						.OnCursorMoved(this, &SShaderEditorBox::OnCursorMoved)
						.OnTextChanged(this, &SShaderEditorBox::OnShaderTextChanged)
						.OnTextCommitted(this, &SShaderEditorBox::OnShadedrTextCommitted)
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
		const int32 LineNum = Marshaller->TextLayout->GetLineCount();
		int32 DiffNum = FMath::Abs(LineNum - CurLineNum);
		if (LineNum >= CurLineNum)
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
				LineNumberSlateTexts.Pop();
				LineNumberData.Pop();
			}
		}
		ShaderCode = InText.ToString();
		LineNumberList->RequestListRefresh();
	}

	void SShaderEditorBox::OnShadedrTextCommitted(const FText& Name, ETextCommit::Type CommitInfo)
	{
		ShaderCode = Name.ToString();
	}

	const FLinearColor LineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };

	void SShaderEditorBox::OnCursorMoved(const FTextLocation& InTextLocaction)
	{
		if (CurCursorLineIndex == InTextLocaction.GetLineIndex())
		{
			return;
		}

		LastCursorLineIndex = CurCursorLineIndex;
		CurCursorLineIndex = InTextLocaction.GetLineIndex();
		if (CurCursorLineIndex < LineNumberSlateTexts.Num())
		{
			LineNumberSlateTexts[CurCursorLineIndex]->SetColorAndOpacity(FLinearColor::White);
			if (LineNumberSlateTexts.IsValidIndex(LastCursorLineIndex))
			{
				LineNumberSlateTexts[LastCursorLineIndex]->SetColorAndOpacity(LineNumberTextColor);
			}
		}
	}

	TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
			.Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
			.ColorAndOpacity(LineNumberTextColor)
			.Text(*Item)
			.Justification(ETextJustify::Right)
			.MinDesiredWidth(15.0f);

		//if the cursor moved to a new line
		if (CurCursorLineIndex == LineNumberSlateTexts.Num())
		{
			LineNumberTextBlock->SetColorAndOpacity(FLinearColor::White);
			if (LineNumberSlateTexts.IsValidIndex(LastCursorLineIndex))
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

	FText SShaderEditorBox::GetShadedrCode() const
	{
		return FText::FromString(ShaderCode);
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