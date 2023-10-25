#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include <Widgets/Text/SlateEditableTextLayout.h>
#include <Widgets/Layout/SScrollBarTrack.h>
#include <Framework/Text/SlateTextRun.h>
#include "GpuApi/GpuApiInterface.h"
#include <Widgets/Layout/SScaleBox.h>
#include <Framework/Commands/GenericCommands.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Framework/Text/TextLayout.h>
#include "ShaderCodeEditorLineHighlighter.h"

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SScrollBar, TSharedPtr<SScrollBarTrack>, Track)
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<FUICommandList>, UICommandList)
CALL_PRIVATE_FUNCTION(SMultiLineEditableText_OnMouseWheel, SMultiLineEditableText, OnMouseWheel,, FReply, const FGeometry&, const FPointerEvent&)

namespace SH
{

	const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
	const FLinearColor HighlightLineNumberTextColor = { 0.7f,0.7f,0.7f,0.9f };

	const FLinearColor NormalLineTipColor = { 1.0f,1.0f,1.0f,0.0f };
	const FLinearColor HighlightLineTipColor = { 1.0f,1.0f,1.0f,0.2f };
	
	const FString FoldMarkerText = TEXT("⇿");
	const FString ErrorMarkerText = TEXT("✘");
	
	void SShaderEditorBox::Construct(const FArguments& InArgs)
	{
        CodeFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		Renderer = InArgs._Renderer;
		SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical);
		SAssignNew(ShaderMultiLineHScrollBar, SScrollBar).Orientation(EOrientation::Orient_Horizontal);

		ShaderMarshaller = MakeShared<FShaderEditorMarshaller>(this, MakeShared<HlslHighLightTokenizer>());
		EffectMarshller = MakeShared<FShaderEditorEffectMarshaller>(this);

		FText InitialShaderText = InArgs._Text;

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
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .FillHeight(1.0f)
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
                            .IsFocusable(false)
							.ConsumeMouseWheel(EConsumeMouseWheel::Never)
                        ]

                        + SHorizontalBox::Slot()
                        .Padding(5, 0, 0, 0)
                        .FillWidth(1.0f)
                        [
                            SNew(SOverlay)
                            + SOverlay::Slot()
                            [
                                SNew(SOverlay)
                                + SOverlay::Slot()
                                [
                                    SAssignNew(ShaderMultiLineEditableText, SMultiLineEditableText)
                                    .Text(InitialShaderText)
                                    .Marshaller(ShaderMarshaller)
                                    .VScrollBar(ShaderMultiLineVScrollBar)
                                    .HScrollBar(ShaderMultiLineHScrollBar)
                                    .OnKeyCharHandler(this, &SShaderEditorBox::OnTextKeyChar)
                                    .OnIsTypedCharValid_Lambda([](const TCHAR InChar) { return true; })
                                ]
                                + SOverlay::Slot()
                                [
                                    SAssignNew(EffectMultiLineEditableText, SMultiLineEditableText)
                                    .IsReadOnly(true)
                                    .Marshaller(EffectMarshller)
                                    .Visibility(EVisibility::HitTestInvisible)
                                ]
                            ]
                            
                            + SOverlay::Slot()
                            [
                                SAssignNew(LineTipList, SListView<LineNumberItemPtr>)
                                .ListItemsSource(&LineNumberData)
                                .SelectionMode(ESelectionMode::None)
                                .OnGenerateRow(this, &SShaderEditorBox::GenerateLineTipForItem)
                                .ScrollbarVisibility(EVisibility::Collapsed)
                                .IsFocusable(false)
                                .Visibility(EVisibility::HitTestInvisible)
                            ]
                            
                        ]
                    ]
                    
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        BuildInfoBar()
                    ]
			
				]
				+ SGridPanel::Slot(1, 0)
				[
					ShaderMultiLineVScrollBar.ToSharedRef()
				]
				+ SGridPanel::Slot(0, 1)
				[
					ShaderMultiLineHScrollBar.ToSharedRef()
				]
			]
				
		];
        
        EffectMultiLineEditableText->SetCanTick(false);
		
		CurEditState = EditState::Normal;

		//Hook, reset the copy/cut behavior.
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		TSharedPtr<FUICommandList>& UICommandList = GetPrivate_FSlateEditableTextLayout_UICommandList(*ShaderEditableTextLayout);
		UICommandList->UnmapAction(FGenericCommands::Get().Copy);
		UICommandList->UnmapAction(FGenericCommands::Get().Cut);

		UICommandList->MapAction(
			FGenericCommands::Get().Copy,
			FExecuteAction::CreateRaw(this, &SShaderEditorBox::CopySelectedText),
			FCanExecuteAction::CreateRaw(this, &SShaderEditorBox::CanCopySelectedText)
		);

		UICommandList->MapAction(
			FGenericCommands::Get().Cut,
			FExecuteAction::CreateRaw(this, &SShaderEditorBox::CutSelectedText),
			FCanExecuteAction::CreateRaw(this, &SShaderEditorBox::CanCutSelectedText)
		);

		FoldingArrowAnim.AddCurve(0, 0.25f, ECurveEaseFunction::Linear);
	}

    FText SShaderEditorBox::GetEditStateText() const
    {
        switch(CurEditState)
        {
        case EditState::Compiling: return FText::FromString(TEXT("COMPILING"));
        case EditState::Failed:    return FText::FromString(TEXT("FAILED"));
        default:                   return FText::FromString(TEXT("NORMAL"));
        }
    }

    FText SShaderEditorBox::GetFontSizeText() const
    {
        return FText::FromString(FString::Printf(TEXT("Font Size:%d"), CodeFontInfo.Size));
    }

    FSlateColor SShaderEditorBox::GetEditStateColor() const
    {
        switch(CurEditState)
        {
        case EditState::Compiling: return FLinearColor{1.0f, 0.5f, 0, 1.0f};
        case EditState::Failed:    return FLinearColor::Red;
        default:                   return FLinearColor{0.6f, 1.0f , 0.1f, 1.0f};
        }
    }

	int32 SShaderEditorBox::GetLineNumber(int32 InLineIndex) const
	{
		if (InLineIndex >= 0 && InLineIndex < LineNumberData.Num())
		{
			return FCString::Atoi(*LineNumberData[InLineIndex]->ToString());
		}
		else
		{
			return INDEX_NONE;
		}
	}

	int32 SShaderEditorBox::GetLineIndex(int32 InLineNumber) const
	{
		int32 LineIndex = LineNumberData.IndexOfByPredicate(
			[InLineNumber](const LineNumberItemPtr& InItem) 
			{ return FCString::Atoi(*InItem->ToString()) == InLineNumber; });
		return LineIndex;
	}

	FText SShaderEditorBox::GetRowColText() const
    {
        const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
        const int32 CursorRow = CursorLocation.GetLineIndex();
        const int32 CursorCol = CursorLocation.GetOffset();
        return FText::FromString(FString::Printf(TEXT("%d:%d"), GetLineNumber(CursorRow), CursorCol));
    }


	void SShaderEditorBox::GenerateInfoBarBox()
	{
		FSlateFontInfo InforBarFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		auto BuildFontSizeMenu = [this]() {
			const int32 MinFontSize = 9;
			const int32 MaxFontSize = 14;

			FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };
			for (int32 i = MinFontSize; i <= MaxFontSize; i++)
			{
				MenuBuilder.AddMenuEntry(
					FText::AsNumber(i),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[=]()
						{
							CodeFontInfo.Size = i;
							ShaderMarshaller->MakeDirty();
							ShaderMultiLineEditableText->Refresh();
						})
						, FCanExecuteAction(),
							FIsActionChecked::CreateLambda(
								[=]()
								{
									return CodeFontInfo.Size == i;
								})),
						NAME_None,
						EUserInterfaceActionType::RadioButton);
			}

			return MenuBuilder.MakeWidget();
		};

		InfoBarBox->ClearChildren();

		InfoBarBox->AddSlot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
				.HasDownArrow(false)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Font(InforBarFontInfo)
					.Text(FText::FromString(TEXT("❖")))
				]
			];

		InfoBarBox->AddSlot()
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SShaderEditorBox::GetEditStateColor)
				[
					SNew(STextBlock)
					.Font(InforBarFontInfo)
					.ColorAndOpacity(FLinearColor::Black)
					.Text(this, &SShaderEditorBox::GetEditStateText)
					.Margin(FMargin{ 10, 0, 10, 0 })
				]
			];

		if (CurEditState == EditState::Failed)
		{
			InfoBarBox->AddSlot()
				.AutoWidth()
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FLinearColor::Red)
					.Text_Lambda([this] {
							if (EffectMarshller->LineNumberToErrorInfo.Num() > 0)
							{
								return FText::FromString(FString::Printf(TEXT("ⓧ %d"), EffectMarshller->LineNumberToErrorInfo.Num()));
							}
							else
							{
								return FText::FromString(TEXT("ⓧ"));
							}
						})
					.Margin(FMargin{5, 0, 0, 0 })
				];
		}

		InfoBarBox->AddSlot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			];

		InfoBarBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(0, 0, 5, 0)
			[
				SNew(SComboButton)
				.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
				.HasDownArrow(false)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Font(InforBarFontInfo)
					.Text(this, &SShaderEditorBox::GetFontSizeText)
				]
				.MenuContent()
				[
					BuildFontSizeMenu()
				]
			];

		InfoBarBox->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SShaderEditorBox::GetEditStateColor)
				[
					SNew(STextBlock)
					.Font(InforBarFontInfo)
					.ColorAndOpacity(FLinearColor::Black)
					.Text(this, &SShaderEditorBox::GetRowColText)
					.Margin(FMargin{ 20, 0, 20, 0 })
				]
			];
	}

	TSharedRef<SWidget> SShaderEditorBox::BuildInfoBar()
    {
		SAssignNew(InfoBarBox, SHorizontalBox);

		GenerateInfoBarBox();

        TSharedRef<SWidget> InfoBar = SNew(SBorder)
        .BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor{0.1f, 0.1f, 0.1f, 0.2f})
        .Padding(0)
        [
			InfoBarBox.ToSharedRef()
        ];
        
        return InfoBar;
    }

	void SShaderEditorBox::CopySelectedText()
	{
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		if (ShaderEditableTextLayout->AnyTextSelected())
		{
			const FTextSelection Selection = ShaderEditableTextLayout->GetSelection();
			FString SelectedText = UnFold(Selection);

			// Copy text to clipboard
			FPlatformApplicationMisc::ClipboardCopy(*SelectedText);
		}
	}

	bool SShaderEditorBox::CanCopySelectedText() const
	{
		bool bCanExecute = true;

		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		// Can't execute if there is no text selected
		if (!ShaderEditableTextLayout->AnyTextSelected())
		{
			bCanExecute = false;
		}

		return bCanExecute;
	}

	void SShaderEditorBox::CutSelectedText()
	{
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		if (ShaderEditableTextLayout->AnyTextSelected())
		{
			SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
			const FTextSelection Selection = ShaderEditableTextLayout->GetSelection();
			FString SelectedText = UnFold(Selection);

			int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
			int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
			for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
			{
				RemoveFoldMarker(LineIndex);
			}
			
			// Copy text to clipboard
			FPlatformApplicationMisc::ClipboardCopy(*SelectedText);

			ShaderEditableTextLayout->DeleteSelectedText();

			ShaderEditableTextLayout->UpdateCursorHighlight();
		}
	}

	bool SShaderEditorBox::CanCutSelectedText() const
	{
		bool bCanExecute = true;

		// Can't paste unless the clipboard has a string in it
		FString ClipboardContent;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
		if (ClipboardContent.IsEmpty())
		{
			bCanExecute = false;
		}

		return bCanExecute;
	}

	FString SShaderEditorBox::FoldMarker::GetTotalFoldedLineTexts() const
	{
		FString TotalFoldedLineTexts;
		int32 SolvedFoldMarkerNum = 0;
		for (int32 i = 0; i < FoldedLineTexts.Num(); i++)
		{
			int32 MarkerOffset = FoldedLineTexts[i].Find(FoldMarkerText);
			if (MarkerOffset != INDEX_NONE)
			{
				TotalFoldedLineTexts += FoldedLineTexts[i].Mid(0, MarkerOffset);
				TotalFoldedLineTexts += ChildFoldMarkers[SolvedFoldMarkerNum].GetTotalFoldedLineTexts();
				TotalFoldedLineTexts += FoldedLineTexts[i].Mid(MarkerOffset + 1);
			}
			else
			{
				TotalFoldedLineTexts += FoldedLineTexts[i];
			}

			if (i < FoldedLineTexts.Num() - 1)
			{
				TotalFoldedLineTexts += LINE_TERMINATOR;
			}

		}
		return TotalFoldedLineTexts;
	}

	int32 SShaderEditorBox::FoldMarker::GetFoledLineCounts() const
	{
		int32 FoledLineCount = FoldedLineTexts.Num() - 1;
		for (const FoldMarker& ChildMarker : ChildFoldMarkers)
		{
			FoledLineCount += ChildMarker.GetFoledLineCounts();
		}
		return FoledLineCount;
	}

	void SShaderEditorBox::UpdateLineNumberData()
	{
		if (IsLineNumberDataDirty)
		{
			LineNumberData.Empty();
			int32 CurTextLayoutLine = ShaderMarshaller->TextLayout->GetLineCount();
			int32 FolendLineCount = 0;
			for (int32 LineIndex = 0; LineIndex < CurTextLayoutLine; LineIndex++)
			{
				LineNumberItemPtr Data = MakeShared<FText>(FText::FromString(FString::FromInt(LineIndex + 1 + FolendLineCount)));
				if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
				{
					const auto& Marker = DisplayedFoldMarkers[*MarkerIndex];
					FolendLineCount += Marker.GetFoledLineCounts();
				}
				LineNumberData.Add(MoveTemp(Data));
			}

			LineNumberList->RequestListRefresh();
			LineTipList->RequestListRefresh();
			MarkLineNumberDataDirty(false);
		}
	}

	void SShaderEditorBox::UpdateLineTipStyle(const double InCurrentTime)
	{
		
		if (ShaderMultiLineEditableText->AnyTextSelected())
		{
			for (int32 i = 0; i < LineNumberData.Num(); i++)
			{
				LineNumberItemPtr ItemData = LineNumberData[i];
				TSharedPtr<STableRow<LineNumberItemPtr>> ItemTableRow = StaticCastSharedPtr<STableRow<LineNumberItemPtr>>(LineTipList->WidgetFromItem(ItemData));
				if (ItemTableRow.IsValid())
				{
					ItemTableRow->SetBorderBackgroundColor(NormalLineTipColor);
				}
			}
			return;
		}

		const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
		const int32 CurLineIndex = CursorLocation.GetLineIndex();

		for (int32 i = 0; i < LineNumberData.Num(); i++)
		{
			LineNumberItemPtr ItemData = LineNumberData[i];
			TSharedPtr<STableRow<LineNumberItemPtr>> ItemTableRow = StaticCastSharedPtr<STableRow<LineNumberItemPtr>>(LineTipList->WidgetFromItem(ItemData));
			if (ItemTableRow.IsValid())
			{
				if (i == CurLineIndex)
				{
					float Speed = 1.8f;
					double AnimatedOpacity = (HighlightLineTipColor.A - 0.1f) * FMath::Pow(FMath::Abs(FMath::Sin(InCurrentTime * Speed)),1.8) + 0.1f;
					ItemTableRow->SetBorderBackgroundColor(FLinearColor{ HighlightLineTipColor.R, HighlightLineTipColor.G, HighlightLineTipColor.B, (float)AnimatedOpacity });
				}
				else
				{
					ItemTableRow->SetBorderBackgroundColor(NormalLineTipColor);
				}
                
                TSharedPtr<STextBlock> DummyTextBlock = StaticCastSharedPtr<STextBlock>(ItemTableRow->GetContent());
                DummyTextBlock->SetFont(CodeFontInfo);
			}
            
		}
	}

	void SShaderEditorBox::UpdateLineNumberHighlight()
	{
		const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
		
		const int32 BeginLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		for (int32 LineIndex = 0; LineIndex < LineNumberData.Num(); LineIndex++)
		{
			LineNumberItemPtr ItemData = LineNumberData[LineIndex];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<SHorizontalBox> ItemWidget = StaticCastSharedPtr<SHorizontalBox>(ItemTableRow->GetContent());
                TSharedPtr<STextBlock> ItemLineNumber = StaticCastSharedRef<STextBlock>(ItemWidget->GetSlot(1).GetWidget());
                
                ItemLineNumber->SetFont(CodeFontInfo);
				if (LineIndex >= BeginLineIndex && LineIndex <= EndLineIndex)
				{
                    ItemLineNumber->SetColorAndOpacity(HighlightLineNumberTextColor);
				}
				else
				{
                    ItemLineNumber->SetColorAndOpacity(NormalLineNumberTextColor);
				}
                
                TSharedPtr<STextBlock> ItemErrorMarker = StaticCastSharedRef<STextBlock>(ItemWidget->GetSlot(0).GetWidget());
                
                ItemErrorMarker->SetFont(CodeFontInfo);

				int32 CurLineNumber = GetLineNumber(LineIndex);
                if(EffectMarshller->LineNumberToErrorInfo.Contains(CurLineNumber))
                {
                    ItemErrorMarker->SetColorAndOpacity(FLinearColor::Red);
                }
				else if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
				{
					int32 FoldedLineCounts = DisplayedFoldMarkers[*MarkerIndex].GetFoledLineCounts();
					for (int32 i = CurLineNumber; i <= CurLineNumber + FoldedLineCounts + 1; i++)
					{
						if (EffectMarshller->LineNumberToErrorInfo.Contains(i))
						{
							ItemErrorMarker->SetColorAndOpacity(FLinearColor::Red);
							break;
						}
					}
				}
                else
                {
                    ItemErrorMarker->SetColorAndOpacity(FLinearColor::Transparent);
                }
			}
		}
	}

	FReply SShaderEditorBox::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		return CallPrivate_SMultiLineEditableText_OnMouseWheel(*ShaderMultiLineEditableText, MyGeometry, MouseEvent);
	}

	FReply SShaderEditorBox::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		Vector2D ScreenSpacePos = MouseEvent.GetScreenSpacePosition();
		Vector2D BoxSpacePos = MyGeometry.AbsoluteToLocal(ScreenSpacePos);
		double AllowableX = LineNumberList->GetTickSpaceGeometry().GetLocalSize().X + 6;
		double AllowableY = MyGeometry.GetLocalSize().Y - InfoBarBox->GetTickSpaceGeometry().GetLocalSize().Y;
		if (BoxSpacePos.X <= AllowableX && BoxSpacePos.Y <= AllowableY)
		{
			FoldingArrowAnim.PlayRelative(AsShared(), true);
		}
		else
		{
			FoldingArrowAnim.PlayRelative(AsShared(), false);
		}
		return FReply::Handled();
	}

	void SShaderEditorBox::OnMouseLeave(const FPointerEvent& MouseEvent)
	{
		FoldingArrowAnim.PlayRelative(AsShared(), false);
	}

	void SShaderEditorBox::UpdateListViewScrollBar()
	{
		TSharedPtr<SScrollBarTrack>& Track = GetPrivate_SScrollBar_Track(*ShaderMultiLineVScrollBar);
		float VOffsetFraction = Track->DistanceFromTop();
		LineNumberList->SetScrollOffset(VOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
		LineTipList->SetScrollOffset(VOffsetFraction * LineTipList->GetNumItemsBeingObserved());
	}

	void SShaderEditorBox::UpdateEffectText()
	{
		EffectMarshller->MakeDirty();
		EffectMultiLineEditableText->Refresh();

		static float LastXoffset;
		TUniquePtr<FSlateEditableTextLayout>& EffectEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*EffectMultiLineEditableText);
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		float XOffsetSizeBetweenLayout = FMath::Max(0.f, (float)EffectEditableTextLayout->GetSize().X - (float)ShaderEditableTextLayout->GetSize().X + LastXoffset);
		ShaderMultiLineEditableText->SetMargin(FMargin{ 0, 0, XOffsetSizeBetweenLayout, 0 });
		LastXoffset = XOffsetSizeBetweenLayout;

		const FGeometry& ShaderMultiLineGeometry = ShaderMultiLineEditableText->GetTickSpaceGeometry();
		const FGeometry& EffectMultiLineGeometry = EffectMultiLineEditableText->GetTickSpaceGeometry();
		FVector2D ShaderScrollOffset = ShaderEditableTextLayout->GetScrollOffset();
		ShaderScrollOffset.X = FMath::Clamp(ShaderScrollOffset.X, 0.0, FMath::Max(0, ShaderEditableTextLayout->GetSize().X - ShaderMultiLineGeometry.GetLocalSize().X));
		ShaderScrollOffset.Y = FMath::Clamp(ShaderScrollOffset.Y, 0.0, FMath::Max(0, ShaderEditableTextLayout->GetSize().Y - ShaderMultiLineGeometry.GetLocalSize().Y));
		FSlateTextLayout* EffectTextLayout = static_cast<FSlateTextLayout*>(EffectMarshller->TextLayout);
		EffectTextLayout->SetVisibleRegion(EffectMultiLineGeometry.GetLocalSize(), ShaderScrollOffset * EffectTextLayout->GetScale());
	}

	void SShaderEditorBox::UpdateFoldingArrow()
	{	
		for (int32 LineIndex = 0; LineIndex < LineNumberData.Num(); LineIndex++)
		{
			LineNumberItemPtr ItemData = LineNumberData[LineIndex];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<SHorizontalBox> ItemWidget = StaticCastSharedPtr<SHorizontalBox>(ItemTableRow->GetContent());
				TSharedPtr<SScaleBox> ItemArrowBox = StaticCastSharedRef<SScaleBox>(ItemWidget->GetSlot(2).GetWidget());
				TSharedPtr<SButton> ItemArrowButton = StaticCastSharedRef<SButton>(ItemArrowBox->GetChildren()->GetChildAt(0));
				if (ShaderMarshaller->FoldingBraceGroups.Contains(LineIndex))
				{
					ItemArrowButton->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowDownButton"));
					ItemArrowButton->SetVisibility(EVisibility::Visible);
					ItemArrowButton->SetBorderBackgroundColor(FLinearColor{1, 1, 1, FoldingArrowAnim.GetLerp()});
				}
				else if (FindFoldMarker(LineIndex))
				{
					ItemArrowButton->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowRightButton"));
					ItemArrowButton->SetVisibility(EVisibility::Visible);
				}
				else
				{
					ItemArrowButton->SetVisibility(EVisibility::Hidden);
				}

			}
		}
	}

	void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		UpdateLineNumberData();
		UpdateListViewScrollBar();
		UpdateLineNumberHighlight();
		UpdateLineTipStyle(InCurrentTime);
		UpdateEffectText();
		UpdateFoldingArrow();
	}

	void SShaderEditorBox::OnShaderTextChanged(const FString& NewShaderSouce)
	{
		FString PixelShaderInput = ShRenderer::DefaultPixelShaderInput;
		FString ShaderResourceDeclaration = Renderer->GetResourceDeclaration();

		TArray<FString> AddedLines;
		int32 AddedLineNum = PixelShaderInput.ParseIntoArrayLines(AddedLines, false) - 1;
		AddedLineNum += ShaderResourceDeclaration.ParseIntoArrayLines(AddedLines, false) - 1;

		FString FinalShaderSource = PixelShaderInput + ShaderResourceDeclaration + NewShaderSouce;
		TRefCountPtr<GpuShader> NewPixelShader = GpuApi::CreateShaderFromSource(ShaderType::PixelShader, MoveTemp(FinalShaderSource), {}, TEXT("MainPS"));
		FString ErrorInfo;
		EffectMarshller->LineNumberToErrorInfo.Reset();
		if (GpuApi::CrossCompileShader(NewPixelShader, ErrorInfo))
		{
			CurEditState = EditState::Normal;
			Renderer->UpdatePixelShader(MoveTemp(NewPixelShader));
		}
		else
		{
			CurEditState = EditState::Failed;
			TArray<ShaderErrorInfo> ErrorInfos = ParseErrorInfoFromDxc(ErrorInfo);
			for (const ShaderErrorInfo& ErrorInfo : ErrorInfos)
			{
				int32 ErrorInfoLineNumber = ErrorInfo.Row - AddedLineNum;
				if (!EffectMarshller->LineNumberToErrorInfo.Contains(ErrorInfoLineNumber))
				{
					int32 LineIndex = GetLineIndex(ErrorInfoLineNumber);
					FString LineText;
					if (LineIndex != INDEX_NONE) {
						ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);
					}

					FString DummyText;
					int32 LineTextNum = LineText.Len();
					for (int32 i = 0; i < LineTextNum; i++)
					{
						DummyText += LineText[i];
					}

					FString DisplayInfo = DummyText + TEXT("    ■ ") + ErrorInfo.Info;
					FTextRange DummyRange{ 0, DummyText.Len() };
					FTextRange ErrorRange{ DummyText.Len(), DisplayInfo.Len() };

					EffectMarshller->LineNumberToErrorInfo.Add(ErrorInfoLineNumber, { MoveTemp(DummyRange), MoveTemp(ErrorRange), MoveTemp(DisplayInfo) });
				}
			}
		}

		if (InfoBarBox.IsValid())
		{
			GenerateInfoBarBox();
		}
	}

	static int32 GetNumSpacesAtStartOfLine(const FString& InLine)
	{
		int32 NumSpaces = 0;
		for (const TCHAR Char : InLine)
		{
			if ((Char != TEXT(' ')))
			{
				break;
			}

			NumSpaces++;
		}

		return NumSpaces;
	}

	static bool IsOpenBrace(const TCHAR& InCharacter)
	{
		return (InCharacter == TEXT('{') || InCharacter == TEXT('[') || InCharacter == TEXT('('));
	}

	static bool IsCloseBrace(const TCHAR& InCharacter)
	{
		return (InCharacter == TEXT('}') || InCharacter == TEXT(']') || InCharacter == TEXT(')'));
	}

	static TCHAR GetMatchedCloseBrace(const TCHAR& InCharacter)
	{
		return InCharacter == TEXT('{') ? TEXT('}') :
			(InCharacter == TEXT('[') ? TEXT(']') : TEXT(')'));
	}

	static bool IsWhiteSpace(const TCHAR& InCharacter)
	{
		return InCharacter == TEXT(' ') || InCharacter == TEXT('\r') || InCharacter == TEXT('\n');
	}

	void SShaderEditorBox::HandleAutoIndent()
	{
		TSharedPtr<SMultiLineEditableText> Text = ShaderMultiLineEditableText;

		const FTextLocation CursorLocation = Text->GetCursorLocation();
		const int32 CurLineIndex = CursorLocation.GetLineIndex();
		const int32 LastLineIndex = CurLineIndex - 1;

		if (LastLineIndex >= 0)
		{
			FString LastLine;
			Text->GetTextLine(LastLineIndex, LastLine);

			const int32 NumSpaces = GetNumSpacesAtStartOfLine(LastLine);

			const int32 NumSpacesForCurrentIndentation = NumSpaces / 4 * 4;
			const FString CurrentIndentation = FString::ChrN(NumSpacesForCurrentIndentation, TEXT(' '));
			const FString NextIndentation = FString::ChrN(NumSpacesForCurrentIndentation + 4, TEXT(' '));

			// See what the open/close curly brace balance is.
			int32 BraceBalance = 0;
			for (const TCHAR Char : LastLine)
			{
				BraceBalance += (Char == TEXT('{'));
				BraceBalance -= (Char == TEXT('}'));
			}

			if (BraceBalance <= 0)
			{
				Text->InsertTextAtCursor(CurrentIndentation);
			}
			else
			{
				Text->InsertTextAtCursor(NextIndentation);

				// Look for an extra close curly brace and auto-indent it as well
				FString CurLine;
				Text->GetTextLine(CurLineIndex, CurLine);
				
				BraceBalance = 0;
				int32 CloseBraceOffset = 0;
				for (const TCHAR Char : CurLine)
				{
					BraceBalance += (Char == TEXT('{'));
					BraceBalance -= (Char == TEXT('}'));

					// Found the first extra '}'
					if (BraceBalance < 0)
					{
						break;
					}

					CloseBraceOffset++;
				}

				if (BraceBalance < 0)
				{
					const FTextLocation SavedCursorLocation = Text->GetCursorLocation();
					const FTextLocation CloseBraceLocation(CurLineIndex, CloseBraceOffset);

					// Create a new line and apply indentation for the close curly brace
					FString NewLineAndIndent(TEXT("\n"));
					NewLineAndIndent.Append(CurrentIndentation);

					Text->GoTo(CloseBraceLocation);
					Text->InsertTextAtCursor(NewLineAndIndent);
					// Recover cursor location
					Text->GoTo(SavedCursorLocation);
				}
			}
		}
	}

	FReply SShaderEditorBox::OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
	{
		TSharedPtr<SMultiLineEditableText> Text = ShaderMultiLineEditableText;
			
		if (Text->IsTextReadOnly())
		{
			return FReply::Unhandled();
		}
		const TCHAR Character = InCharacterEvent.GetCharacter();

		if (Character == TEXT('\b'))
		{
			if (!Text->AnyTextSelected())
			{
				// if we are deleting a single open brace
				// look for a matching close brace following it and delete the close brace as well
				const FTextLocation CursorLocation = Text->GetCursorLocation();
				const int Offset = CursorLocation.GetOffset();
				FString Line;
				Text->GetTextLine(CursorLocation.GetLineIndex(), Line);

				if (Line.IsValidIndex(Offset) && Line.IsValidIndex(Offset - 1))
				{
					if (IsOpenBrace(Line[Offset - 1]))
					{
						if (Line[Offset] == GetMatchedCloseBrace(Line[Offset - 1]))
						{
							SMultiLineEditableText::FScopedEditableTextTransaction ScopedTransaction(Text);
							Text->SelectText(FTextLocation(CursorLocation, -1), FTextLocation(CursorLocation, 1));
							Text->DeleteSelectedText();
							Text->GoTo(FTextLocation(CursorLocation, -1));
							return FReply::Handled();
						}
					}

					if (Line[Offset - 1] == FoldMarkerText[0])
					{
						OnFold(GetLineNumber(CursorLocation.GetLineIndex()));
						return FReply::Handled();
					}

				}
				
			}
			else
			{
				const FTextSelection Selection = Text->GetSelection();
				int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
				int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
				for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
				{
					RemoveFoldMarker(LineIndex);
				}
			}

			return FReply::Unhandled();
		}
		else if (Character == TEXT('\t'))
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);

			const FTextLocation CursorLocation = Text->GetCursorLocation();

			const bool bShouldIncreaseIndentation = InCharacterEvent.GetModifierKeys().IsShiftDown() ? false : true;

			// When there is no text selected, shift tab should also decrease line indentation
			const bool bShouldIndentLine = Text->AnyTextSelected() || (!Text->AnyTextSelected() && !bShouldIncreaseIndentation);

			if (bShouldIndentLine)
			{
				// Indent the whole line if there is a text selection
				const FTextSelection Selection = Text->GetSelection();
				const FTextLocation SelectionStart =
					CursorLocation == Selection.GetBeginning() ? Selection.GetEnd() : Selection.GetBeginning();

				// Shift the selection according to the new indentation
				FTextLocation NewCursorLocation;
				FTextLocation NewSelectionStart;

				const int32 StartLine = Selection.GetBeginning().GetLineIndex();
				const int32 EndLine = Selection.GetEnd().GetLineIndex();

				for (int32 Index = StartLine; Index <= EndLine; Index++)
				{
					const FTextLocation LineStart(Index, 0);
					Text->GoTo(LineStart);

					FString Line;
					Text->GetTextLine(Index, Line);
					const int32 NumSpaces = GetNumSpacesAtStartOfLine(Line);
					const int32 NumExtraSpaces = NumSpaces % 4;

					// Tab to nearest 4.
					int32 NumSpacesForIndentation;
					if (bShouldIncreaseIndentation)
					{
						NumSpacesForIndentation = NumExtraSpaces == 0 ? 4 : 4 - NumExtraSpaces;
						Text->InsertTextAtCursor(FString::ChrN(NumSpacesForIndentation, TEXT(' ')));
					}
					else
					{
						NumSpacesForIndentation = NumExtraSpaces == 0 ? FMath::Min(4, NumSpaces) : NumExtraSpaces;
						Text->SelectText(LineStart, FTextLocation(LineStart, NumSpacesForIndentation));
						Text->DeleteSelectedText();
					}

					const int32 CursorShiftDirection = bShouldIncreaseIndentation ? 1 : -1;
					const int32 CursorShift = NumSpacesForIndentation * CursorShiftDirection;

					if (Index == CursorLocation.GetLineIndex())
					{
						NewCursorLocation = FTextLocation(CursorLocation, CursorShift);
					}

					if (Index == SelectionStart.GetLineIndex())
					{
						NewSelectionStart = FTextLocation(SelectionStart, CursorShift);
					}
				}

				Text->SelectText(NewSelectionStart, NewCursorLocation);
			}
			else
			{
				FString Line;
				Text->GetCurrentTextLine(Line);

				const int32 Offset = CursorLocation.GetOffset();

				// Tab to nearest 4.
				if (ensure(bShouldIncreaseIndentation))
				{
					const int32 NumSpacesForIndentation = 4 - Offset % 4;
					Text->InsertTextAtCursor(FString::ChrN(NumSpacesForIndentation, TEXT(' ')));
				}
			}

			return FReply::Handled();
		}
		else if (Character == TEXT('\n') || Character == TEXT('\r'))
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);

			// at this point, the text after the text cursor is already in a new line
			HandleAutoIndent();

			return FReply::Handled();
		}
		else if (IsOpenBrace(Character))
		{
			const TCHAR CloseBrace = GetMatchedCloseBrace(Character);
			const FTextLocation CursorLocation = Text->GetCursorLocation();
			FString Line;
			Text->GetCurrentTextLine(Line);

			bool bShouldAutoInsertBraces = false;

			if (CursorLocation.GetOffset() < Line.Len())
			{
				const TCHAR NextChar = Text->GetCharacterAt(CursorLocation);

				if (IsWhiteSpace(NextChar))
				{
					bShouldAutoInsertBraces = true;
				}
				else if (IsCloseBrace(NextChar))
				{
					int32 BraceBalancePrior = 0;
					for (int32 Index = 0; Index < CursorLocation.GetOffset() && Index < Line.Len(); Index++)
					{
						BraceBalancePrior += (Line[Index] == Character);
						BraceBalancePrior -= (Line[Index] == CloseBrace);
					}

					int32 BraceBalanceLater = 0;
					for (int32 Index = CursorLocation.GetOffset(); Index < Line.Len(); Index++)
					{
						BraceBalanceLater += (Line[Index] == Character);
						BraceBalanceLater -= (Line[Index] == CloseBrace);
					}

					if (BraceBalancePrior >= -BraceBalanceLater)
					{
						bShouldAutoInsertBraces = true;
					}
				}
			}
			else
			{
				bShouldAutoInsertBraces = true;
			}

			// auto insert if we have more open braces
			// on the left side than close braces on the right side
			if (bShouldAutoInsertBraces)
			{
				// auto insert the matched close brace
				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);
				Text->InsertTextAtCursor(FString::Chr(Character));
				Text->InsertTextAtCursor(FString::Chr(CloseBrace));
				const FTextLocation NewCursorLocation(Text->GetCursorLocation(), -1);
				Text->GoTo(NewCursorLocation);
				return FReply::Handled();
			}

			return FReply::Unhandled();
		}
		else if (IsCloseBrace(Character))
		{
			if (!Text->AnyTextSelected())
			{
				const FTextLocation CursorLocation = Text->GetCursorLocation();
				FString Line;
				Text->GetTextLine(CursorLocation.GetLineIndex(), Line);

				const int32 Offset = CursorLocation.GetOffset();

				if (Line.IsValidIndex(Offset))
				{
					if (Line[Offset] == Character)
					{
						// avoid creating a duplicated close brace and simply
						// advance the cursor
						Text->GoTo(FTextLocation(CursorLocation, 1));
						return FReply::Handled();
					}
				}
			}

			return FReply::Unhandled();
		}
		else
		{
			// Let SMultiLineEditableText::OnKeyChar handle it.
			return FReply::Unhandled();
		}
	}

	TOptional<int32> SShaderEditorBox::FindFoldMarker(int32 InIndex) const
	{
		for (int32 i = 0; i < DisplayedFoldMarkers.Num(); i++)
		{
			if (DisplayedFoldMarkers[i].LineIndex == InIndex)
			{
				return i;
			}
		}
		return {};
	}

	FString SShaderEditorBox::UnFold(const FTextSelection& DisplayedTextRange)
	{
		FString TextAfterUnfolding;
		
		const TArray< FTextLayout::FLineModel >& Lines = ShaderMarshaller->TextLayout->GetLineModels();
		int32 StartLineIndex = DisplayedTextRange.GetBeginning().GetLineIndex();
		int32 EndLineIndex = DisplayedTextRange.GetEnd().GetLineIndex();
		int32 StartOffset = DisplayedTextRange.GetBeginning().GetOffset();
		int32 EndOffset = DisplayedTextRange.GetEnd().GetOffset();

		for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
		{
			FString LineText;
			if (LineIndex == StartLineIndex)
			{
				LineText = Lines[LineIndex].Text->Mid(StartOffset);
			}
			else if(LineIndex == EndLineIndex)
			{
				LineText = Lines[LineIndex].Text->Mid(0, EndOffset + 1);
			}
			else
			{
				LineText = *Lines[LineIndex].Text;
			}

			TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
			if (MarkerIndex)
			{
				const auto& Marker = DisplayedFoldMarkers[*MarkerIndex];
				TextAfterUnfolding += LineText.Mid(0, Marker.Offset);
				TextAfterUnfolding += Marker.GetTotalFoldedLineTexts();
				TextAfterUnfolding += LineText.Mid(Marker.Offset + 1);
			}
			else
			{
				TextAfterUnfolding += *LineText;
			}

			if (LineIndex < EndLineIndex) {
				TextAfterUnfolding += LINE_TERMINATOR;
			}
		}
	
		return TextAfterUnfolding;
	}

	void SShaderEditorBox::RemoveFoldMarker(int32 InIndex)
	{
		for (int32 i = 0; i < DisplayedFoldMarkers.Num(); i++)
		{
			if (DisplayedFoldMarkers[i].LineIndex == InIndex)
			{
				DisplayedFoldMarkers.RemoveAt(i);
				return;
			}
		}
	}

	FReply SShaderEditorBox::OnFold(int32 LineNumber)
	{
		int32 LineIndex = GetLineIndex(LineNumber);
		FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
		TArray< FTextLayout::FLineModel >& Lines = const_cast<TArray<FTextLayout::FLineModel>&>(ShaderTextLayout->GetLineModels());
		
		SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
		TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
		if (!MarkerIndex)
		{	
			auto BraceGroup = ShaderMarshaller->FoldingBraceGroups[LineIndex];
			int32 FoldedBeginningRow = BraceGroup.LeftBracePos.Row;
			int32 FoldedBeginningCol = BraceGroup.LeftBracePos.Col;
			int32 FoldedEndRow = BraceGroup.RightBracePos.Row;
			int32 FoldedEndCol = BraceGroup.RightBracePos.Col;

			FTextLayout::FLineModel& EndLine = Lines[FoldedEndRow];
			TArray<FString> FoldedTexts;
			
			FString EndLineRemains;
			if (EndLine.Text->Len() > FoldedEndCol)
			{
				EndLineRemains = EndLine.Text->Mid(FoldedEndCol);
			}
			FoldedTexts.Add(*EndLine.Text->Mid(0, EndLine.Text->Len() - EndLineRemains.Len()));
			ShaderTextLayout->RemoveLine(FoldedEndRow);

			for (int32 LineIndex = FoldedEndRow - 1; LineIndex > FoldedBeginningRow; LineIndex--)
			{
				FoldedTexts.Add(*Lines[LineIndex].Text);
				ShaderTextLayout->RemoveLine(LineIndex);
			}

			FTextLayout::FLineModel& BeginLine = Lines[FoldedBeginningRow];
			FoldedTexts.Add(*BeginLine.Text->Mid(FoldedBeginningCol + 1));
			ShaderTextLayout->RemoveAt(FTextLocation(FoldedBeginningRow, FoldedBeginningCol + 1), BeginLine.Text->Len() - FoldedBeginningCol - 1);

			for (int32 i = 0; i < FoldedTexts.Num() / 2; i++)
			{
				FoldedTexts.Swap(i, FoldedTexts.Num() - 1 - i);
			}

			//Join with the fold marker
			FoldMarker Marker = { FoldedBeginningRow, BeginLine.Text->Len(), MoveTemp(FoldedTexts) };
			ShaderTextLayout->InsertAt(FTextLocation{ FoldedBeginningRow, BeginLine.Text->Len() }, FoldMarkerText + EndLineRemains);
			for (int32 LineIndex = FoldedBeginningRow + 1; LineIndex <= FoldedEndRow; LineIndex++)
			{
				if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
				{
					Marker.ChildFoldMarkers.Add(DisplayedFoldMarkers[*MarkerIndex]);
					RemoveFoldMarker(LineIndex);
				}
			}
			DisplayedFoldMarkers.Add(MoveTemp(Marker));
			DisplayedFoldMarkers.Sort([](const FoldMarker& A, const FoldMarker& B) { return A.LineIndex < B.LineIndex; });
			//Finally set the marker style by the marshaller. 
			ShaderMarshaller->MakeDirty();
			
			const FTextLocation CurCursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
			if (CurCursorLocation.GetLineIndex() >= FoldedBeginningRow && CurCursorLocation.GetLineIndex() <= FoldedEndRow)
			{
				const FTextLocation NewCursorLocation(FoldedBeginningRow, BeginLine.Text->Len());
				ShaderMultiLineEditableText->GoTo(NewCursorLocation);
			}
		
		}
		else
		{
			const FoldMarker* Marker = &DisplayedFoldMarkers[*MarkerIndex];
			TArray<FString> FoldedLineText = Marker->FoldedLineTexts;
			FTextLayout::FLineModel& BeginLine = Lines[LineIndex];
	
			int32 MarkerCol = BeginLine.Text->Find(FoldMarkerText);

			FString TextOnMarkerRight = BeginLine.Text->Mid(MarkerCol + 1);
			ShaderTextLayout->RemoveAt(FTextLocation{ LineIndex, MarkerCol }, BeginLine.Text->Len() - MarkerCol);
			ShaderTextLayout->InsertAt(FTextLocation{ LineIndex, MarkerCol }, FoldedLineText[0]);
	
			for (int32 i = 1; i < FoldedLineText.Num(); i++)
			{
				TSharedPtr<FString> NewLineText;
				if (i == FoldedLineText.Num() - 1) {
					NewLineText = MakeShared<FString>(FoldedLineText[i] + TextOnMarkerRight);
				}
				else {
					NewLineText = MakeShared<FString>(FoldedLineText[i]);
				}
				TSharedRef<IRun> Run = FSlateTextRun::Create(FRunInfo(), NewLineText.ToSharedRef(), FTextBlockStyle{}, {0, NewLineText->Len()});
				
				FTextLayout::FLineModel LineModel(NewLineText.ToSharedRef());
				LineModel.Runs.Add(FTextLayout::FRunModel(Run));

				int32 NewLineIndex = LineIndex + i;
				Lines.Insert(MoveTemp(LineModel), NewLineIndex);
			}

			for (int32 i = 0; i < Marker->ChildFoldMarkers.Num(); i++)
			{
				//The marshaller will help to set the correct line index and offset, we just need to keep relative order between markers.
				DisplayedFoldMarkers.Insert(Marker->ChildFoldMarkers[i], *MarkerIndex + i + 1);
			}

			ShaderMarshaller->MakeDirty();
			RemoveFoldMarker(LineIndex);
		}

		return FReply::Handled();
	}

	TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
			.Font(CodeFontInfo)
			.ColorAndOpacity(NormalLineNumberTextColor)
			.Text(*Item)
			.Justification(ETextJustify::Right)
			.MinDesiredWidth(15.0f);
		
		int32 LineNumber = FCString::Atoi(*(*Item).ToString());

		TSharedPtr<SButton> FoldingArrow = SNew(SButton)
			.ContentPadding(FMargin(0, 0))
			.ButtonStyle(FShaderHelperStyle::Get(), "ArrowDownButton")
			.Visibility(EVisibility::Hidden)
			.OnClicked(this, &SShaderEditorBox::OnFold, LineNumber);
		
		FoldingArrow->SetPadding(FMargin{});
		
		return SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
			.Content()
			[
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
				.AutoWidth()
                [
                    SNew(STextBlock)
                    .Font(CodeFontInfo)
                    .ColorAndOpacity(FLinearColor::Transparent)
                    .Text(FText::FromString(ErrorMarkerText))
                ]

                + SHorizontalBox::Slot()
				.AutoWidth()
                [
                    LineNumberTextBlock.ToSharedRef()
                ]

				+ SHorizontalBox::Slot()
				.Padding(5, 0, 0, 0)
				[
					SNew(SScaleBox)
					[
						FoldingArrow.ToSharedRef()
					]
				]
				
			];
	}

	TSharedRef<ITableRow> SShaderEditorBox::GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		//DummyTextBlock is used to keep the same layout as LineNumber and MultiLineEditableText.
		TSharedPtr<STextBlock> DummyTextBlock = SNew(STextBlock)
			.Font(CodeFontInfo)
			.Visibility(EVisibility::Hidden);

		TSharedPtr<STableRow<LineNumberItemPtr>> LineTip = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineTipItemStyle"))
			.Content()
			[
				DummyTextBlock.ToSharedRef()
			];

	
		LineTip->SetBorderBackgroundColor(NormalLineTipColor);
		
		return LineTip.ToSharedRef();
	}

	FShaderEditorMarshaller::FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<HlslHighLightTokenizer> InTokenizer)
		: OwnerWidget(InOwnerWidget)
        , TextLayout(nullptr)
        , Tokenizer(MoveTemp(InTokenizer))
	{
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Number, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNumberText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Keyword, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorKeywordText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Punctuation, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorPunctuationText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::BuildtinFunc, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorBuildtinFuncText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::BuildtinType, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorBuildtinTypeText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Identifier, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Preprocess, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorPreprocessText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Comment, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorCommentText"));
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Other, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText"));
	}

	void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;

		TArray<HlslHighLightTokenizer::BraceGroup> BraceGroups;
		TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines = Tokenizer->Tokenize(SourceString, BraceGroups);

		FoldingBraceGroups.Empty();
		for (const auto& BraceGroup : BraceGroups)
		{
			if (BraceGroup.LeftBracePos.Row != BraceGroup.RightBracePos.Row && !FoldingBraceGroups.Contains(BraceGroup.LeftBracePos.Row)) {
				FoldingBraceGroups.Add(BraceGroup.LeftBracePos.Row, BraceGroup);
			}
		}

		TArray<SShaderEditorBox::FoldMarker> NewFoldMarkers;
		for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); LineIndex++)
		{
			TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(TokenizedLines[LineIndex].LineRange.BeginIndex, TokenizedLines[LineIndex].LineRange.Len()));
			int32 MarkerOffset = LineText->Find(FoldMarkerText);
			if (MarkerOffset != INDEX_NONE)
			{
				NewFoldMarkers.Add({ LineIndex, MarkerOffset});
			}
		}

		check(NewFoldMarkers.Num() == OwnerWidget->DisplayedFoldMarkers.Num());
		
		TArray<FTextLineHighlight> Highlights;
		for (int32 i = 0; i < OwnerWidget->DisplayedFoldMarkers.Num(); i++)
		{
			auto& Marker = OwnerWidget->DisplayedFoldMarkers[i];
			Marker.LineIndex = NewFoldMarkers[i].LineIndex;
			Marker.Offset = NewFoldMarkers[i].Offset;

			Highlights.Emplace(Marker.LineIndex, FTextRange{ Marker.Offset, Marker.Offset + 1 }, -1, FoldMarkerHighLighter::Create());
		}


		TArray<FTextLayout::FNewLineData> LinesToAdd;
		for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); LineIndex++)
		{
			TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(TokenizedLines[LineIndex].LineRange.BeginIndex, TokenizedLines[LineIndex].LineRange.Len()));
			
			TArray<TSharedRef<IRun>> Runs;
			int32 RunBeginIndexInAllString = TokenizedLines[LineIndex].LineRange.BeginIndex;
			TOptional<int32> MarkerIndex = OwnerWidget->FindFoldMarker(LineIndex);
			for (const HlslHighLightTokenizer::Token& Token : TokenizedLines[LineIndex].Tokens)
			{
				FTextBlockStyle RunTextStyle = TokenStyleMap[Token.Type];
				RunTextStyle.SetFont(OwnerWidget->GetFontInfo());
				FTextRange NewTokenRange{ Token.Range.BeginIndex - RunBeginIndexInAllString, Token.Range.EndIndex - RunBeginIndexInAllString };

				if (MarkerIndex && LineText->Mid(NewTokenRange.BeginIndex, 1) == FoldMarkerText)
				{
					RunTextStyle.SetColorAndOpacity(FLinearColor::Gray);
				}

				Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, MoveTemp(RunTextStyle), MoveTemp(NewTokenRange)));
			}

			LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
		}

		TextLayout->AddLines(MoveTemp(LinesToAdd));

		//Update fold markers highlight.
		for (const FTextLineHighlight& Highlight : Highlights)
		{
			TextLayout->AddLineHighlight(Highlight);
		}

		FTextSelection UnFoldingRange = FTextSelection{ {0, 0}, {LinesToAdd.Num() - 1, LinesToAdd[LinesToAdd.Num() - 1].Text->Len() - 1} };
		FString TextAfterUnfolding = OwnerWidget->UnFold(MoveTemp(UnFoldingRange));
		OwnerWidget->OnShaderTextChanged(TextAfterUnfolding);
		OwnerWidget->MarkLineNumberDataDirty(true);
	}

	void FShaderEditorMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	void FShaderEditorEffectMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;
		SubmitEffectText();
	}

	void FShaderEditorEffectMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	void FShaderEditorEffectMarshaller::SubmitEffectText()
	{
		TArray<FTextLayout::FNewLineData> LinesToAdd;
		for (int32 LineIndex = 0; LineIndex < OwnerWidget->GetCurDisplayLineCount(); LineIndex++)
		{
			TArray<TSharedRef<IRun>> Runs;
			FTextBlockStyle ErrorInfoStyle = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorErrorInfoText");
			ErrorInfoStyle.SetFont(OwnerWidget->GetFontInfo());

			FTextBlockStyle DummyInfoStyle = FTextBlockStyle{}
				.SetFont(OwnerWidget->GetFontInfo())
				.SetColorAndOpacity(FLinearColor{ 0, 0, 0, 0 });

			int32 CurLineNumber = OwnerWidget->GetLineNumber(LineIndex);
			if (LineNumberToErrorInfo.Contains(CurLineNumber))
			{
				TSharedRef<FString> TotalInfo = MakeShared<FString>(LineNumberToErrorInfo[CurLineNumber].TotalInfo);

				TSharedRef<IRun> DummyRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, DummyInfoStyle, LineNumberToErrorInfo[CurLineNumber].DummyRange);
				Runs.Add(MoveTemp(DummyRun));

				TSharedRef<IRun> ErrorRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, ErrorInfoStyle, LineNumberToErrorInfo[CurLineNumber].ErrorRange);
				Runs.Add(MoveTemp(ErrorRun));

				LinesToAdd.Emplace(MoveTemp(TotalInfo), MoveTemp(Runs));
			}
			else
			{
				TSharedRef<FString> Empty = MakeShared<FString>();
				Runs.Add(FSlateTextRun::Create(FRunInfo(), Empty, ErrorInfoStyle));

				LinesToAdd.Emplace(Empty, MoveTemp(Runs));
			}
		}

		TextLayout->AddLines(MoveTemp(LinesToAdd));
	}
}
