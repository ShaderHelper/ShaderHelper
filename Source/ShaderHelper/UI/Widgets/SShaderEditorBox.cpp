#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include <Widgets/Text/SlateEditableTextLayout.h>
#include  <Widgets/Layout/SScrollBarTrack.h>

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, SlateEditableTextTypes::FCursorInfo, CursorInfo)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TOptional<FTextLocation>, SelectionStart)
STEAL_PRIVATE_MEMBER(SScrollBar, TSharedPtr<SScrollBarTrack>, Track)

namespace SH
{

	const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
	const FLinearColor HighlightLineNumberTextColor = { FLinearColor::White };
	
	void SShaderEditorBox::Construct(const FArguments& InArgs)
	{
		SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical);
		TSharedPtr<SScrollBar> HScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Horizontal);

		Marshaller = MakeShared<FShaderEditorMarshaller>();

	    CurLineNum = 1;
		LineNumberData = { MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))) };

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
						.IsFocusable(false)
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
						.OnTextChanged(this, &SShaderEditorBox::OnShaderTextChanged)
						.OnTextCommitted(this, &SShaderEditorBox::OnShadedrTextCommitted)
						.VScrollBar(ShaderMultiLineVScrollBar)
						.HScrollBar(HScrollBar)
					]
			
				]
				+ SGridPanel::Slot(1, 0)
				[
					ShaderMultiLineVScrollBar.ToSharedRef()
				]
				+ SGridPanel::Slot(0, 1)
				[
					HScrollBar.ToSharedRef()
				]
			]
				
		];
	}

	void SShaderEditorBox::UpdateLineNumberHighlight()
	{
		TUniquePtr<FSlateEditableTextLayout>& SlateTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		SlateEditableTextTypes::FCursorInfo& CursorInfo = GetPrivate_FSlateEditableTextLayout_CursorInfo(*SlateTextLayout);
		TOptional<FTextLocation>& SelectionStart = GetPrivate_FSlateEditableTextLayout_SelectionStart(*SlateTextLayout);
		const FTextLocation CursorInteractionPosition = CursorInfo.GetCursorInteractionLocation();
		const FTextLocation SelectionLocation = SelectionStart.Get(CursorInteractionPosition);
		const FTextSelection Selection(SelectionLocation, CursorInteractionPosition);
		
		const int32 BeginLineNumber = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineNumber = Selection.GetEnd().GetLineIndex();
		for (int32 i = 0; i < LineNumberData.Num(); i++)
		{
			LineNumberItemPtr ItemData = LineNumberData[i];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<STextBlock> ItemWidget = StaticCastSharedPtr<STextBlock>(ItemTableRow->GetContent());
				if (i >= BeginLineNumber && i <= EndLineNumber)
				{
					ItemWidget->SetColorAndOpacity(HighlightLineNumberTextColor);
				}
				else
				{
					ItemWidget->SetColorAndOpacity(NormalLineNumberTextColor);
				}
			}
		}
	}

	void SShaderEditorBox::UpdateLineNumberListViewScrollBar()
	{
		TSharedPtr<SScrollBarTrack>& Track = GetPrivate_SScrollBar_Track(*ShaderMultiLineVScrollBar);
		float VOffsetFraction = Track->DistanceFromTop();
		LineNumberList->SetScrollOffset(VOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
	}

	void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		UpdateLineNumberListViewScrollBar();
		UpdateLineNumberHighlight();
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

	TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
			.Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
			.ColorAndOpacity(NormalLineNumberTextColor)
			.Text(*Item)
			.Justification(ETextJustify::Right)
			.MinDesiredWidth(15.0f);
		
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