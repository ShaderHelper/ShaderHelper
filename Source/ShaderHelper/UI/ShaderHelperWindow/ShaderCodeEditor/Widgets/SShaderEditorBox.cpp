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

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SScrollBar, TSharedPtr<SScrollBarTrack>, Track)
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<FSlateTextLayout>, TextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<FUICommandList>, UICommandList)
CALL_PRIVATE_FUNCTION(SMultiLineEditableText_OnMouseWheel, SMultiLineEditableText, OnMouseWheel,, FReply, const FGeometry&, const FPointerEvent&)

namespace SH
{

	const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
	const FLinearColor HighlightLineNumberTextColor = { 0.7f,0.7f,0.7f,0.9f };

	const FLinearColor NormalLineTipColor = { 1.0f,1.0f,1.0f,0.0f };
	const FLinearColor HighlightLineTipColor = { 1.0f,1.0f,1.0f,0.2f };
	
	
	void SShaderEditorBox::Construct(const FArguments& InArgs)
	{
        CodeFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		Renderer = InArgs._Renderer;
		SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical);
		SAssignNew(ShaderMultiLineHScrollBar, SScrollBar).Orientation(EOrientation::Orient_Horizontal);

		ShaderMarshaller = MakeShared<FShaderEditorMarshaller>(this, MakeShared<HlslHighLightTokenizer>());
		EffectMarshller = MakeShared<FShaderEditorEffectMarshaller>(this);

		TArray<FTextRange> LineRanges;
		FTextRange::CalculateLineRangesFromString(InArgs._Text.ToString(), LineRanges);
		if (LineRanges.Num() > 0)
		{
			int32 LineNums = LineRanges.Num();
			CurLineNum = 0;
			while (LineNums--)
			{
				CurLineNum++;
				LineNumberData.Add(MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))));
			}
		}
		else
		{
			CurLineNum = 1;
			LineNumberData.Add(MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))));
		}
        
        CurEditState = EditState::Normal;
		FullShaderSource = InArgs._Text.ToString();
		
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
                                    .Text(InArgs._Text)
                                    .Marshaller(ShaderMarshaller)
                                    .OnTextChanged(this, &SShaderEditorBox::OnShaderTextChanged)
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

		//Hook, reset the copy behavior.
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		TSharedPtr<FUICommandList>& UICommandList = GetPrivate_FSlateEditableTextLayout_UICommandList(*ShaderEditableTextLayout);
		UICommandList->UnmapAction(FGenericCommands::Get().Copy);

		UICommandList->MapAction(
			FGenericCommands::Get().Copy,
			FExecuteAction::CreateRaw(this, &SShaderEditorBox::CopySelectedText),
			FCanExecuteAction::CreateRaw(this, &SShaderEditorBox::CanCopySelectedText)
		);
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

	FText SShaderEditorBox::GetRowColText() const
    {
        const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
        const int32 CursorRow = CursorLocation.GetLineIndex() + 1;
        const int32 CursorCol = CursorLocation.GetOffset();
        return FText::FromString(FString::Printf(TEXT("%d:%d"), CursorRow, CursorCol));
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
							return FText::FromString(FString::Printf(TEXT("ⓧ %d"), EffectMarshller->LineNumToErrorInfo.Num()));
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
			FString SelectedText = ShaderEditableTextLayout->GetSelectedText().ToString();

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
		
		const int32 BeginLineNumber = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineNumber = Selection.GetEnd().GetLineIndex();
		for (int32 i = 0; i < LineNumberData.Num(); i++)
		{
			LineNumberItemPtr ItemData = LineNumberData[i];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<SHorizontalBox> ItemWidget = StaticCastSharedPtr<SHorizontalBox>(ItemTableRow->GetContent());
                TSharedPtr<STextBlock> ItemLineNumber = StaticCastSharedRef<STextBlock>(ItemWidget->GetSlot(1).GetWidget());
                
                ItemLineNumber->SetFont(CodeFontInfo);
				if (i >= BeginLineNumber && i <= EndLineNumber)
				{
                    ItemLineNumber->SetColorAndOpacity(HighlightLineNumberTextColor);
				}
				else
				{
                    ItemLineNumber->SetColorAndOpacity(NormalLineNumberTextColor);
				}
                
                TSharedPtr<STextBlock> ItemErrorMarker = StaticCastSharedRef<STextBlock>(ItemWidget->GetSlot(0).GetWidget());
                
                ItemErrorMarker->SetFont(CodeFontInfo);
                if(EffectMarshller->LineNumToErrorInfo.Contains(i + 1))
                {
                    ItemErrorMarker->SetColorAndOpacity(FLinearColor::Red);
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
		double AllowableX = LineNumberList->GetTickSpaceGeometry().GetLocalSize().X + 5;
		double AllowableY = MyGeometry.GetLocalSize().Y - InfoBarBox->GetTickSpaceGeometry().GetLocalSize().Y + 5;
		if (BoxSpacePos.X <= AllowableX && BoxSpacePos.Y <= AllowableY)
		{
			UpdateFold(true);
		}
		else
		{
			UpdateFold(false);
		}
		return FReply::Handled();
	}

	void SShaderEditorBox::OnMouseLeave(const FPointerEvent& MouseEvent)
	{
		UpdateFold(false);
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
		TSharedPtr<FSlateTextLayout>& EffectTextLayout = GetPrivate_FSlateEditableTextLayout_TextLayout(*EffectEditableTextLayout);
		EffectTextLayout->SetVisibleRegion(EffectMultiLineGeometry.GetLocalSize(), ShaderScrollOffset * EffectTextLayout->GetScale());
	}

	void SShaderEditorBox::UpdateFold(bool IsShow)
	{
		for (int32 i = 0; i < LineNumberData.Num(); i++)
		{
			LineNumberItemPtr ItemData = LineNumberData[i];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<SHorizontalBox> ItemWidget = StaticCastSharedPtr<SHorizontalBox>(ItemTableRow->GetContent());
				TSharedPtr<SScaleBox> ItemArrowBox = StaticCastSharedRef<SScaleBox>(ItemWidget->GetSlot(2).GetWidget());
				TSharedPtr<SButton> ItemArrowButton = StaticCastSharedRef<SButton>(ItemArrowBox->GetChildren()->GetChildAt(0));
				if (ShaderMarshaller->FoldingBraceGroups.Contains(i + 1))
				{
					if (FoldedLineNumbers.Contains(i + 1))
					{
						ItemArrowButton->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowRightButton"));
						ItemArrowButton->SetVisibility(EVisibility::Visible);
					}
					else
					{
						ItemArrowButton->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowDownButton"));
						ItemArrowButton->SetVisibility(IsShow ? EVisibility::Visible : EVisibility::Collapsed);
					}
				}

			}
		}
	}

	void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		UpdateListViewScrollBar();
		UpdateLineNumberHighlight();
		UpdateLineTipStyle(InCurrentTime);
		UpdateEffectText();
	}

	void SShaderEditorBox::OnShaderTextChanged(const FText& InText)
	{
		const int32 LineNum = ShaderMarshaller->TextLayout->GetLineCount();
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
		LineNumberList->RequestListRefresh();
		LineTipList->RequestListRefresh();

		FString NewShaderSouce = InText.ToString();
		
		TRefCountPtr<GpuShader> NewPixelShader = GpuApi::CreateShaderFromSource(ShaderType::PixelShader, MoveTemp(NewShaderSouce), {}, TEXT("MainPS"));
		FString ErrorInfo;
		EffectMarshller->LineNumToErrorInfo.Reset();
		if (GpuApi::CrossCompileShader(NewPixelShader, ErrorInfo))
		{
            CurEditState = EditState::Normal;
			GenerateInfoBarBox();
			
			Renderer->UpdatePixelShader(MoveTemp(NewPixelShader));
		}
		else
		{
			CurEditState = EditState::Failed;
			GenerateInfoBarBox();
			           
			TArray<ShaderErrorInfo> ErrorInfos = ParseErrorInfoFromDxc(ErrorInfo);
			for (const ShaderErrorInfo& ErrorInfo : ErrorInfos)
			{
				if (!EffectMarshller->LineNumToErrorInfo.Contains(ErrorInfo.Row))
				{
					FString LineText;
					ShaderMultiLineEditableText->GetTextLine(ErrorInfo.Row - 1, LineText);

					FString DummyText;
					int32 LineTextNum = LineText.Len();
                    for(int32 i = 0; i < LineTextNum; i++)
                    {
                        DummyText += LineText[i];
                    }

					FString DisplayInfo = DummyText + TEXT("    ■ ") + ErrorInfo.Info;
                    FTextRange DummyRange{0, DummyText.Len()};
                    FTextRange ErrorRange{DummyText.Len(), DisplayInfo.Len()};
                    
                    EffectMarshller->LineNumToErrorInfo.Add(ErrorInfo.Row, {MoveTemp(DummyRange), MoveTemp(ErrorRange), MoveTemp(DisplayInfo)});
				}
			}
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

	void SShaderEditorBox::HandleAutoIndent() const
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

	FReply SShaderEditorBox::OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) const
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
			.ButtonStyle(FAppStyle::Get(), "InvisibleButton")
			.OnClicked_Lambda(
				[=]()
				{
					if (!FoldedLineNumbers.Contains(LineNumber))
					{
						auto BraceGroup = ShaderMarshaller->FoldingBraceGroups[LineNumber];
						

					//	FoledLineNumberToLineText[]
						
						FoldedLineNumbers.Add(LineNumber);
					}
					else
					{
						FoldedLineNumbers.Remove(LineNumber);
					}
					return FReply::Handled();
				});
		
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
                    .Text(FText::FromString(TEXT("✘")))
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

		TArray<FTextLayout::FNewLineData> LinesToAdd;
		for (const HlslHighLightTokenizer::TokenizedLine& TokenizedLine : TokenizedLines)
		{
			TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(TokenizedLine.LineRange.BeginIndex, TokenizedLine.LineRange.Len()));
			TArray<TSharedRef<IRun>> Runs;
			int32 RunBeginIndexInAllString = TokenizedLine.LineRange.BeginIndex;
			for (const HlslHighLightTokenizer::Token& Token : TokenizedLine.Tokens)
			{
				FTextBlockStyle RunTextStyle = TokenStyleMap[Token.Type];
				RunTextStyle.SetFont(OwnerWidget->GetFontInfo());
				FTextRange NewTokenRange{ Token.Range.BeginIndex - RunBeginIndexInAllString, Token.Range.EndIndex - RunBeginIndexInAllString };
				Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, MoveTemp(RunTextStyle), MoveTemp(NewTokenRange)));
			}

			LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
		}
		TextLayout->AddLines(MoveTemp(LinesToAdd));
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
		for(int32 i = 0; i < OwnerWidget->GetCurLineNum(); i++)
		{
			TArray<TSharedRef<IRun>> Runs;
			FTextBlockStyle ErrorInfoStyle = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorErrorInfoText");
            ErrorInfoStyle.SetFont(OwnerWidget->GetFontInfo());
            
            FTextBlockStyle DummyInfoStyle = FTextBlockStyle{}
                .SetFont(OwnerWidget->GetFontInfo())
                .SetColorAndOpacity(FLinearColor{0, 0, 0, 0});
            
			if (LineNumToErrorInfo.Contains(i + 1))
			{
                TSharedRef<FString> TotalInfo = MakeShared<FString>(LineNumToErrorInfo[i + 1].TotalInfo);
                
                TSharedRef<IRun> DummyRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, DummyInfoStyle, LineNumToErrorInfo[i + 1].DummyRange);
                Runs.Add(MoveTemp(DummyRun));
                
                TSharedRef<IRun> ErrorRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, ErrorInfoStyle, LineNumToErrorInfo[i + 1].ErrorRange);
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
