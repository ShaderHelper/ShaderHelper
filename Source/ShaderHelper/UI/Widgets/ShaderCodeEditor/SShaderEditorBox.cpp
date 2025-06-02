#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include <Widgets/Text/SlateEditableTextLayout.h>
#include <Widgets/Layout/SScrollBarTrack.h>
#include <Framework/Text/SlateTextRun.h>
#include "GpuApi/GpuRhi.h"
#include <Widgets/Layout/SScaleBox.h>
#include <Framework/Commands/GenericCommands.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Framework/Text/TextLayout.h>
#include "ShaderCodeEditorLineHighlighter.h"
#include "UI/Widgets/Misc/CommonCommands.h"
#include "Editor/ShaderHelperEditor.h"
#include "magic_enum.hpp"
#include <Widgets/Text/SlateTextBlockLayout.h>
#include <Framework/Text/SlateTextHighlightRunRenderer.h>
#include <Fonts/FontMeasure.h>

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<FUICommandList>, UICommandList)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>, CursorLineHighlighter)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, SlateEditableTextTypes::FCursorInfo, CursorInfo)
STEAL_PRIVATE_MEMBER(STextBlock, TUniquePtr< FSlateTextBlockLayout >, TextLayoutCache)
STEAL_PRIVATE_MEMBER(FSlateTextBlockLayout, TSharedPtr<FSlateTextLayout>, TextLayout)
CALL_PRIVATE_FUNCTION(SMultiLineEditableText_OnMouseWheel, SMultiLineEditableText, OnMouseWheel,, FReply, const FGeometry&, const FPointerEvent&)

using namespace FW;

namespace SH
{

const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
const FLinearColor HighlightLineNumberTextColor = { 0.7f,0.7f,0.7f,0.9f };

const FLinearColor NormalLineTipColor = { 1.0f,1.0f,1.0f,0.0f };
const FLinearColor HighlightLineTipColor = { 1.0f,1.0f,1.0f,0.2f };

const FString FoldMarkerText = TEXT("⇿");
const FString ErrorMarkerText = TEXT("✘");

    TokenBreakIterator::TokenBreakIterator(FShaderEditorMarshaller* InMarshaller)
        : Marshaller(InMarshaller)
    {
        
    }

    void TokenBreakIterator::SetString(FString&& InString)
    {
        InternalString = MoveTemp(InString);
        ResetToBeginning();
    }

    void TokenBreakIterator::SetStringRef(FStringView InString)
    {
        InternalString = InString;
        ResetToBeginning();
    }

    int32 TokenBreakIterator::GetCurrentPosition() const
    {
        return CurrentPosition;
    }

    int32 TokenBreakIterator::ResetToBeginning()
    {
        return CurrentPosition = 0;
    }

    int32 TokenBreakIterator::ResetToEnd()
    {
        return CurrentPosition = InternalString.Len();
    }

    int32 TokenBreakIterator::MoveToPrevious()
    {
        return MoveToCandidateBefore(CurrentPosition);
    }

    int32 TokenBreakIterator::MoveToNext()
    {
        return MoveToCandidateAfter(CurrentPosition);
    }

    int32 TokenBreakIterator::MoveToCandidateBefore(const int32 InIndex)
    {
        TArray<HlslHighLightTokenizer::BraceGroup> _;
        TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, _, true);
        const HlslHighLightTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
        for(const auto& Token : TokenLine.Tokens)
        {
            if(InIndex > Token.Range.BeginIndex && InIndex <= Token.Range.EndIndex)
            {
                CurrentPosition = Token.Range.BeginIndex;
                break;
            }
        }
        return CurrentPosition >= InIndex ? INDEX_NONE : CurrentPosition;
    }

    int32 TokenBreakIterator::MoveToCandidateAfter(const int32 InIndex)
    {
        TArray<HlslHighLightTokenizer::BraceGroup> _;
        TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, _, true);
        const HlslHighLightTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
        for(const auto& Token : TokenLine.Tokens)
        {
            if(InIndex >= Token.Range.BeginIndex && InIndex < Token.Range.EndIndex)
            {
                CurrentPosition = Token.Range.EndIndex;
                break;
            }
        }
        return CurrentPosition <= InIndex ? INDEX_NONE : CurrentPosition;
    }
    
    SShaderEditorBox::~SShaderEditorBox()
    {
        bQuitISense = true;
        ISenseEvent->Trigger();
        ISenseThread->Join();
        FPlatformProcess::ReturnSynchEventToPool(ISenseEvent);
    }

    static bool FuzzyMatch(const FString& Candidate, const FString& Token)
    {
        int32 SearchPos = 0;
        for (int32 i = 0; i < Token.Len(); ++i)
        {
            TCHAR C = FChar::ToLower(Token[i]);
            bool bFound = false;
            while (SearchPos < Candidate.Len())
            {
                if (FChar::ToLower(Candidate[SearchPos]) == C)
                {
                    bFound = true;
                    ++SearchPos;
                    break;
                }
                ++SearchPos;
            }
            if (!bFound)
                return false;
        }
        return true;
    }

    void SShaderEditorBox::Construct(const FArguments& InArgs)
    {
        CodeFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		ShaderAssetObj = InArgs._ShaderAssetObj;
        SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical);
        SAssignNew(ShaderMultiLineHScrollBar, SScrollBar).Orientation(EOrientation::Orient_Horizontal);

        ShaderMarshaller = MakeShared<FShaderEditorMarshaller>(this, MakeShared<HlslHighLightTokenizer>());
        EffectMarshller = MakeShared<FShaderEditorEffectMarshaller>(this);

        FText InitialShaderText = FText::FromString(ShaderAssetObj->EditorContent);
        CurrentEditorSource = ShaderAssetObj->EditorContent;
        CurrentShaderSource = CurrentEditorSource;
        
        ISenseEvent = FPlatformProcess::GetSynchEventFromPool();
        ISenseThread = MakeUnique<FThread>(TEXT("ISenseThread"), [this]{
            while(!bQuitISense)
            {
                ISenseTask Task;
                while(ISenseQueue.Dequeue(Task));
                if(Task.ShaderDesc)
                {
					TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(Task.ShaderDesc.value());
                    ISenseTU TU{Shader->GetProcessedSourceText(), Shader->GetIncludeDirs()};
                    ErrorInfos = TU.GetDiagnostic();
                    
                    CandidateInfos.Reset();
                    if(!Task.CursorToken.IsEmpty())
                    {
                        CandidateInfos = TU.GetCodeComplete(Task.Row, Task.Col);
                        if(Task.IsMemberAccess == false)
                        {
                            for(const auto& Candidate : DefaultCandidates())
                            {
                                CandidateInfos.AddUnique(Candidate);
                            }
                        }
 
                        for(auto It = CandidateInfos.CreateIterator(); It; ++It)
                        {
                            if(Task.IsMemberAccess)
                            {
                                if((*It).Text.Find("operator") != INDEX_NONE)
                                {
                                    It.RemoveCurrent();
                                    continue;
                                }
                                else if((*It).Kind == HLSL::CandidateKind::Type)
                                {
                                    It.RemoveCurrent();
                                    continue;
                                }
                            }
                            
                            if(Task.CursorToken != ".")
                            {
                                if(!FuzzyMatch((*It).Text, *Task.CursorToken))
                                {
                                    It.RemoveCurrent();
                                }
                            }
                        }
                        
                        CandidateInfos.Sort([&](const ShaderCandidateInfo& A, const ShaderCandidateInfo& B){
                            auto Token = Task.CursorToken;
                            bool AStarts = A.Text.StartsWith(Token, ESearchCase::CaseSensitive);
                            bool BStarts = B.Text.StartsWith(Token, ESearchCase::CaseSensitive);
                            if (AStarts != BStarts)
                                return AStarts > BStarts;
                            bool AStartsIC = A.Text.StartsWith(Token, ESearchCase::IgnoreCase);
                            bool BStartsIC = B.Text.StartsWith(Token, ESearchCase::IgnoreCase);
                            if (AStartsIC != BStartsIC)
                                return AStartsIC > BStartsIC;
                            int32 AIndex = A.Text.Find(Token, ESearchCase::IgnoreCase);
                            int32 BIndex = B.Text.Find(Token, ESearchCase::IgnoreCase);
                            if (AIndex != BIndex)
                                return (AIndex >= 0 ? AIndex : MAX_int32) < (BIndex >= 0 ? BIndex : MAX_int32);
                            if (A.Text.Len() != B.Text.Len())
                                return A.Text.Len() < B.Text.Len();
                            return A.Text.Compare(B.Text) < 0;
                        });
                    }
                    
                    bRefreshIsense.store(true, std::memory_order_release);
                }

                ISenseEvent->Wait();
                FPlatformProcess::SleepNoStats(0.01f);
            }
        });
        
        auto CustomScrollBar = SNew(SScrollBar).Padding(0)
            .Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar"));

        ChildSlot
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
            .Padding(0)
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
                        .FillWidth(1.0f)
                        [
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SAssignNew(ShaderMultiLineEditableText, SMultiLineEditableText)
								.Text(InitialShaderText)
								.Marshaller(ShaderMarshaller)
								.VScrollBar(ShaderMultiLineVScrollBar)
								.HScrollBar(ShaderMultiLineHScrollBar)
								.OnKeyCharHandler(this, &SShaderEditorBox::HandleKeyChar)
								.OnKeyDownHandler(this, &SShaderEditorBox::HandleKeyDown)
								.CreateSlateTextLayout_Lambda([this](SWidget* InOwner, FTextBlockStyle InDefaultTextStyle){
									return MakeShared<ShaderTextLayout>(InOwner, InDefaultTextStyle, ShaderMarshaller.Get());
								})
								.OnIsTypedCharValid_Lambda([](const TCHAR InChar) { return true; })
								.OnCursorMoved_Lambda([this](const FTextLocation& NewCursorLocation){
									if(!bKeyChar) {
										bTryComplete = false;
									}
									bKeyChar = false;
								})
							]
							+ SOverlay::Slot()
							[
								SAssignNew(EffectMultiLineEditableText, SMultiLineEditableText)
								.IsReadOnly(true)
								.Marshaller(EffectMarshller)
								.Visibility(EVisibility::HitTestInvisible)
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
							+ SOverlay::Slot()
							[
								SAssignNew(CodeCompletionCanvas, SCanvas)
								+ SCanvas::Slot()
								.Size_Lambda([this]{
									float Height = CustomCursorHighlighter->ScaledLineHeight * CandidateItems.Num();
									float HSize = FMath::Min(200.0f, Height);
									return FVector2D{240, HSize};
								})
								.Position_Lambda([this]{
									FVector2D TipPos = CustomCursorHighlighter->ScaledCursorPos;
									TipPos.Y += CustomCursorHighlighter->ScaledLineHeight;
									FVector2D Area = ShaderMultiLineEditableText->GetTickSpaceGeometry().GetLocalSize();
									float HSize = FMath::Min(200.0f, CustomCursorHighlighter->ScaledLineHeight * CandidateItems.Num());
									if(TipPos.Y + HSize > Area.Y)
									{
										TipPos.Y -= CustomCursorHighlighter->ScaledLineHeight;
										TipPos.Y -= HSize;
									}
									return TipPos;
								})
								[
									SNew(SBorder)
									.Visibility_Lambda([this]{
										return bTryComplete? EVisibility::Visible : EVisibility::Collapsed;
									})
									.BorderImage(FAppStyle::Get().GetBrush("Brushes.AccentGray"))
									.Padding(1.0)
									[
										SNew(SHorizontalBox)
										+SHorizontalBox::Slot()
										[
											SAssignNew(CodeCompletionList, SListView<CandidateItemPtr>)
											.ListItemsSource(&CandidateItems)
											.SelectionMode(ESelectionMode::Single)
											.OnGenerateRow(this, &SShaderEditorBox::GenerateCodeCompletionItem)
											.EnableAnimatedScrolling(true)
											.ConsumeMouseWheel(EConsumeMouseWheel::Always)
											.ExternalScrollbar(CustomScrollBar)
											.OnSelectionChanged_Lambda([this](CandidateItemPtr SelectedItem, ESelectInfo::Type){
												CurSelectedCandidate = SelectedItem;
											})
											.OnMouseButtonClick_Lambda([this](CandidateItemPtr ClickedItem){
												InsertCompletionText(ClickedItem->Text);
												FSlateApplication::Get().SetUserFocus(0, ShaderMultiLineEditableText);
											})
										]
										+SHorizontalBox::Slot()
										.AutoWidth()
										[
											CustomScrollBar
										]
									]
								]
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
        
        TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
        ShaderMultiLineEditableTextLayout = ShaderEditableTextLayout.Get();
        ShaderMultiLineEditableTextLayout->ClearUndoStateOnFocusLost = false;
        ShaderMultiLineEditableTextLayout->MakeUndoState = [this] {
            auto UndoState = MakeUnique<ShaderUndoState>();
            ShaderMultiLineEditableTextLayout->InitUndoState(*UndoState);
            UndoState->FoldMarkers = VisibleFoldMarkers;
			UndoState->BreakPointLines = BreakPointLines;
            return UndoState;
        };

        ShaderMultiLineEditableTextLayout->CopyOverride = [this] { CopySelectedText(); };
        ShaderMultiLineEditableTextLayout->CutOverride = [this] { CutSelectedText(); };
        ShaderMultiLineEditableTextLayout->PasteOverride = [this] { PasteText(); };
		ShaderMultiLineEditableTextLayout->OnBeginEditTransaction = [this] { OnBeginEditTransaction(); };
        
        TSharedPtr<FUICommandList>& UICommandList = GetPrivate_FSlateEditableTextLayout_UICommandList(*ShaderEditableTextLayout);
        UICommandList->MapAction(
            CommonCommands::Get().Save,
            FExecuteAction::CreateLambda([this] {
				ShaderAssetObj->Save();
                Compile();
            })
        );

        FoldingArrowAnim.AddCurve(0, 0.25f, ECurveEaseFunction::Linear);

        //Hook the default FCursorLineHighlighter.
        TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>& CursorLineHighlighter = GetPrivate_FSlateEditableTextLayout_CursorLineHighlighter(*ShaderEditableTextLayout);
        SlateEditableTextTypes::FCursorInfo& CursorInfo = GetPrivate_FSlateEditableTextLayout_CursorInfo(*ShaderEditableTextLayout);
        
        CustomCursorHighlighter = CursorHightLighter::Create(&CursorInfo);
        CursorLineHighlighter = CustomCursorHighlighter;
        
        CurEditState = ShaderAssetObj->bCompilationSucceed ? EditState::Succeed : EditState::Failed;
    }

	void SShaderEditorBox::OnBeginEditTransaction()
	{
		SelectionBeforeEdit = ShaderMultiLineEditableText->GetSelection();
	}

    FText SShaderEditorBox::GetEditStateText() const
    {
        switch(CurEditState)
        {
        case EditState::Compiling: return LOCALIZATION("COMPILING");
        case EditState::Failed:    return LOCALIZATION("FAILED");
        default:                   return LOCALIZATION("SUCCEED");
        }
    }

    FText SShaderEditorBox::GetFontSizeText() const
    {
        return LOCALIZATION("FontSize");
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
        const int32 CursorCol = CursorLocation.GetOffset() + 1;
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
                        [this, i]()
                        {
                            CodeFontInfo.Size = i;
                            ShaderMarshaller->MakeDirty();
                            ShaderMultiLineEditableText->Refresh();
                            EffectMarshller->MakeDirty();
                            EffectMultiLineEditableText->Refresh();
                        }),
                        FCanExecuteAction(),
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
                SNew(SButton)
                .IsFocusable(false)
                .OnClicked_Lambda([this]{
                    Compile();
                    return FReply::Handled();
                })
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
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Font(InforBarFontInfo)
                    .ColorAndOpacity(FLinearColor::Black)
                    .Text(this, &SShaderEditorBox::GetEditStateText)
                    .Margin(FMargin{ 10, 0, 10, 0 })
                ]
            ];

        InfoBarBox->AddSlot()
            .FillWidth(1.0f)
            [
                SNullWidget::NullWidget
            ];

        InfoBarBox->AddSlot()
            .AutoWidth()
            .HAlign(HAlign_Right)
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
                .VAlign(VAlign_Center)
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
        [
            InfoBarBox.ToSharedRef()
        ];
        
        return InfoBar;
    }

    void SShaderEditorBox::PasteText()
    {
        FString PastedText;
        FPlatformApplicationMisc::ClipboardPaste(PastedText);
        ShaderMultiLineEditableText->InsertTextAtCursor(PastedText);
        
        ShaderMarshaller->MakeDirty();
        ShaderMultiLineEditableText->Refresh();
    }

    void SShaderEditorBox::CopySelectedText()
    {
        if (ShaderMultiLineEditableTextLayout->AnyTextSelected())
        {
            const FTextSelection Selection = ShaderMultiLineEditableTextLayout->GetSelection();
            FString SelectedText = UnFoldText(Selection);

            // Copy text to clipboard
            FPlatformApplicationMisc::ClipboardCopy(*SelectedText);
        }
    }

    void SShaderEditorBox::CutSelectedText()
    {
        if (ShaderMultiLineEditableTextLayout->AnyTextSelected())
        {
            SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
            const FTextSelection Selection = ShaderMultiLineEditableTextLayout->GetSelection();
            FString SelectedText = UnFoldText(Selection);

            int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
            int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
            for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
            {
                RemoveFoldMarker(LineIndex);
            }
            
            // Copy text to clipboard
            FPlatformApplicationMisc::ClipboardCopy(*SelectedText);

            ShaderMultiLineEditableTextLayout->DeleteSelectedText();

            ShaderMultiLineEditableTextLayout->UpdateCursorHighlight();
        }
    }

    FString FoldMarker::GetTotalFoldedLineTexts() const
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

    int32 FoldMarker::GetFoldedLineCounts() const
    {
        int32 FoledLineCount = FoldedLineTexts.Num() - 1;
        for (const FoldMarker& ChildMarker : ChildFoldMarkers)
        {
            FoledLineCount += ChildMarker.GetFoldedLineCounts();
        }
        return FoledLineCount;
    }

    void SShaderEditorBox::UpdateLineNumberData()
    {
		int32 OldLineCount = LineNumberData.Num();
		int32 StartLineNumber = GetLineNumber(SelectionBeforeEdit.GetBeginning().GetLineIndex());
		int32 EndLineNumber = GetLineNumber(SelectionBeforeEdit.GetEnd().GetLineIndex());
		
        LineNumberData.Reset();
        int32 CurTextLayoutLine = ShaderMarshaller->TextLayout->GetLineCount();
        int32 FoldedLineCount = 0;

        for (int32 LineIndex = 0; LineIndex < CurTextLayoutLine; LineIndex++)
        {
            LineNumberItemPtr Data = MakeShared<FText>(FText::FromString(FString::FromInt(LineIndex + 1 + FoldedLineCount)));
            if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
            {
                const auto& Marker = VisibleFoldMarkers[*MarkerIndex];
				FoldedLineCount += Marker.GetFoldedLineCounts();
            }
			
			int32 LineNumber = FCString::Atoi(*(*Data).ToString());
			MaxLineNumber = FMath::Max(MaxLineNumber, LineNumber);
            LineNumberData.Add(MoveTemp(Data));
        }
		
		int32 DeltaLineCount = CurTextLayoutLine - OldLineCount;
		bool IsRedoOrUndo = ShaderMultiLineEditableTextLayout && ShaderMultiLineEditableTextLayout->CurrentUndoLevel >= 0;
		if(!IsFoldEditTransaction && !IsRedoOrUndo && DeltaLineCount != 0)
		{
			for(auto It = BreakPointLines.CreateIterator(); It; ++It)
			{
				if(StartLineNumber < *It)
				{
					if(EndLineNumber >= *It)
					{
						It.RemoveCurrent();
					}
					else
					{
						*It += DeltaLineCount;
					}
				}
			}
		}

        if(LineNumberList) {
            LineNumberList->RequestListRefresh();
        }
        
        if(LineTipList){
            LineTipList->RequestListRefresh();
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
        float VOffsetFraction = ShaderMultiLineVScrollBar->DistanceFromTop();
        LineNumberList->SetScrollOffset(VOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
        LineTipList->SetScrollOffset(VOffsetFraction * LineTipList->GetNumItemsBeingObserved());
    }

    void SShaderEditorBox::UpdateEffectText()
    {
        //Sync the error text location when scrolling ShaderMultiLineEditableText.
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

    void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
        
        if(bRefreshIsense.load(std::memory_order_acquire))
        {
            RefreshLineNumberToErrorInfo();
            RefreshCodeCompletionTip();
            bRefreshIsense.store(false, std::memory_order_relaxed);
        }
        
        UpdateListViewScrollBar();
        UpdateEffectText();
    }

    void SShaderEditorBox::OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
    {
        bTryComplete = false;
    }

    FReply SShaderEditorBox::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
    {
        return FReply::Handled().SetUserFocus(ShaderMultiLineEditableText.ToSharedRef());
    }

    void SShaderEditorBox::InsertCompletionText(const FString& InText)
    {
        const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
        const int32 CursorRow = CursorLocation.GetLineIndex();
        const int32 CursorCol = CursorLocation.GetOffset();
        FString CursorLineText;
        ShaderMultiLineEditableText->GetTextLine(CursorRow, CursorLineText);
        FString CursorLeftText = CursorLineText.Mid(0, CursorCol);
        int32 InsertCol = CursorLeftText.Find(*CurToken, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        check(InsertCol != INDEX_NONE);
        if(CurToken == ".")
        {
            InsertCol += 1;
        }
        
        bTryComplete = false;
        FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
        const FTextLocation NewCursorLocation(CursorRow, InsertCol);
        ShaderMultiLineEditableText->GoTo(NewCursorLocation);
        ShaderTextLayout->RemoveAt({CursorRow, InsertCol}, CurToken.Len());
        ShaderMultiLineEditableText->InsertTextAtCursor(InText);
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateCodeCompletionItem(CandidateItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
		auto CandidateText = SNew(STextBlock).Font(CodeFontInfo)
		.Text(FText::FromString(Item->Text))
		.OverflowPolicy(ETextOverflowPolicy::Ellipsis);
		
		{
			//STextBlock's built-in HighlightText only supports highlighting continuous substring
			CandidateText->ComputeDesiredSize(1.0f);
			TUniquePtr<FSlateTextBlockLayout>& TextLayoutCache = GetPrivate_STextBlock_TextLayoutCache(*CandidateText);
			TSharedPtr<FSlateTextLayout>& TextLayout = GetPrivate_FSlateTextBlockLayout_TextLayout(*TextLayoutCache);
			TArray<FTextRunRenderer> TextHighlights;
			TSharedPtr<ISlateRunRenderer> TextHighlighter = FSlateTextHighlightRunRenderer::Create();
			int32 TextLen = Item->Text.Len();
            int32 TokenLen = CurToken.Len();
            int32 TextPos = 0;
            int32 TokenPos = 0;
            while (TextPos < TextLen && TokenPos < TokenLen)
            {
                if (FChar::ToLower(Item->Text[TextPos]) == FChar::ToLower(CurToken[TokenPos]))
                {
                    TextHighlights.Add(FTextRunRenderer(0, FTextRange(TextPos, TextPos + 1), TextHighlighter.ToSharedRef()));
                    ++TokenPos;
                }
                ++TextPos;
            }
			TextLayout->SetRunRenderers(TextHighlights);
		}
		
        return SNew(STableRow<CandidateItemPtr>, OwnerTable)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .FillWidth(0.75f)
            [
				CandidateText
            ]
            +SHorizontalBox::Slot()
            .FillWidth(0.25f)
            [
                SNew(STextBlock)
                .Justification(ETextJustify::Right)
                .Text(FText::FromString(FString{magic_enum::enum_name(Item->Kind).data()}))
            ]
        ];
    }

    void SShaderEditorBox::RefreshCodeCompletionTip()
    {
        CandidateItems.Reset();
        for(const auto& CandidateInfo : CandidateInfos)
        {
            CandidateItems.Add(MakeShared<ShaderCandidateInfo>(CandidateInfo));
            if(CandidateItems.Num() >= 40)
            {
                break;
            }
        }
        
        if(CandidateItems.Num() > 0)
        {
            CodeCompletionCanvas->SlatePrepass();
            CodeCompletionList->RequestListRefresh();
            CodeCompletionList->SetSelection(CandidateItems[0]);
        }

    }

    void SShaderEditorBox::RefreshLineNumberToErrorInfo()
    {
        EffectMarshller->LineNumberToErrorInfo.Reset();
        for (const ShaderErrorInfo& ErrorInfo : ErrorInfos)
        {
            int32 ErrorInfoLineNumber = ErrorInfo.Row - ShaderAssetObj->GetExtraLineNum();
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

                FString DisplayInfo = DummyText + TEXT("  ") + ErrorInfo.Info;
                FTextRange DummyRange{ 0, DummyText.Len() };
                FTextRange ErrorRange{ DummyText.Len(), DisplayInfo.Len() };

                EffectMarshller->LineNumberToErrorInfo.Add(ErrorInfoLineNumber, { MoveTemp(DummyRange), MoveTemp(ErrorRange), MoveTemp(DisplayInfo) });
            }
        }
        
        if(EffectMultiLineEditableText && EffectMarshller)
        {
            EffectMarshller->MakeDirty();
            EffectMultiLineEditableText->Refresh();
        }

    }

    void SShaderEditorBox::Compile()
    {
		//TODO Async and show "Compiling" state
        TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(ShaderAssetObj->GetShaderDesc(CurrentShaderSource));
        FString ErrorInfo;
        if (GGpuRhi->CompileShader(Shader, ErrorInfo))
        {
			ShaderAssetObj->Shader = Shader;
			ShaderAssetObj->bCompilationSucceed = true;
            CurEditState = EditState::Succeed;
        }
        else
        {
			int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();
			ErrorInfo = AdjustErrorLineNumber(ErrorInfo, -AddedLineNum);
			SH_LOG(LogShader, Error, TEXT("Compilation failed:\n%s"), *ErrorInfo);
			ShaderAssetObj->bCompilationSucceed = false;
            CurEditState = EditState::Failed;
        }
    }

    void SShaderEditorBox::OnShaderTextChanged(const FString& InShaderSouce)
    {
        FString NewShaderSource = InShaderSouce.Replace(TEXT("\r\n"), TEXT("\n"));
        if (NewShaderSource != ShaderAssetObj->EditorContent)
        {
			ShaderAssetObj->EditorContent = NewShaderSource;
			ShaderAssetObj->MarkDirty(NewShaderSource != ShaderAssetObj->SavedEditorContent);
        }
        
        CurrentShaderSource = NewShaderSource;

		ISenseTask Task{};
		Task.ShaderDesc = ShaderAssetObj->GetShaderDesc(CurrentShaderSource);
        
        if(bTryComplete)
        {
            //Note: The cursor has advanced here. CursorLocation starts from 0, but libclang 1
            const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
            const int32 CursorRow = CursorLocation.GetLineIndex();
            const int32 CursorCol = CursorLocation.GetOffset();
			int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();
            
            FString CurLineText;
            ShaderMultiLineEditableText->GetTextLine(CursorRow, CurLineText);
            FString CursorLeft = CurLineText.Mid(0, CursorCol);
            
            TArray<HlslHighLightTokenizer::BraceGroup> _;
            TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines = ShaderMarshaller->Tokenizer->Tokenize(CursorLeft, _, true);
            FString LeftToken = CursorLeft.Mid(TokenizedLines[0].Tokens.Last().Range.BeginIndex, TokenizedLines[0].Tokens.Last().Range.Len());
            if(TokenizedLines[0].Tokens.Num() > 1)
            {
                FString LeftToken2 = CursorLeft.Mid(TokenizedLines[0].Tokens.Last(1).Range.BeginIndex, TokenizedLines[0].Tokens.Last(1).Range.Len());
                if(LeftToken2 == ".")
                {
                    Task.IsMemberAccess = true;
                }
            }
            CurToken = LeftToken;
            
            Task.Row = GetLineNumber(CursorRow) + AddedLineNum;
            if(LeftToken == ".")
            {
                Task.IsMemberAccess = true;
                Task.Col = CursorCol + 1;
            }
            else
            {
                Task.Col = CursorCol + 1 - LeftToken.Len();
            }
            Task.CursorToken = MoveTemp(LeftToken);
        }
        
        ISenseQueue.Enqueue(MoveTemp(Task));
        ISenseEvent->Trigger();
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

    FReply SShaderEditorBox::HandleKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
    {
        const FKey Key = InKeyEvent.GetKey();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		//Process tool bar commands.
		if(ShEditor->GetUICommandList().IsValid())
		{
			ShEditor->GetUICommandList()->ProcessCommandBindings(InKeyEvent);
		}

        if(bTryComplete && CandidateItems.Num())
        {
            if(Key == EKeys::Up)
            {
                int32 Index = CandidateItems.Find(CurSelectedCandidate);
                if(Index == 0)
                {
                    Index = CandidateItems.Num();
                }
                auto TargetItem = CandidateItems[Index - 1];
                CodeCompletionList->SetSelection(TargetItem);
                CodeCompletionList->RequestScrollIntoView(TargetItem);
                return FReply::Handled();
            }
            else if(Key == EKeys::Down)
            {
                int32 Index = CandidateItems.Find(CurSelectedCandidate);
                auto TargetItem = CandidateItems[(Index + 1) % CandidateItems.Num()];
                CodeCompletionList->SetSelection(TargetItem);
                CodeCompletionList->RequestScrollIntoView(TargetItem);
                return FReply::Handled();
            }
            else if(Key == EKeys::Enter || Key == EKeys::Tab)
            {
                return FReply::Handled();
            }
        }
            
        // Let SMultiLineEditableText::OnKeyDown handle it.
        return FReply::Unhandled();
    }

    FReply SShaderEditorBox::HandleKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
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

                if (Line.IsValidIndex(Offset - 1))
                {
                    if (IsOpenBrace(Line[Offset - 1]))
                    {
                        if (Line.IsValidIndex(Offset) && Line[Offset] == GetMatchedCloseBrace(Line[Offset - 1]))
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
        else if (Character == TEXT('\t') or Character == 0x19)
        {
            if(bTryComplete && CandidateItems.Num())
            {
                InsertCompletionText(CurSelectedCandidate->Text);
            }
            else
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
            }
    
            return FReply::Handled();
        }
        else if (Character == TEXT('\n') || Character == TEXT('\r'))
        {
            if(bTryComplete && CandidateItems.Num())
            {
                InsertCompletionText(CurSelectedCandidate->Text);
            }
            else
            {
                SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);
                
                // at this point, the text after the text cursor is already in a new line
                HandleAutoIndent();
            }

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

                if (FChar::IsWhitespace(NextChar))
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
		else if (Character == '"')
		{
			Text->InsertTextAtCursor("\"\"");
			const FTextLocation CursorLocation = Text->GetCursorLocation();
			Text->GoTo(FTextLocation{CursorLocation.GetLineIndex(), CursorLocation.GetOffset() - 1});
			return FReply::Handled();
		}
        else if(FChar::IsIdentifier(Character) || Character == TEXT('.'))
        {
            bTryComplete = true;
            bKeyChar = true;
        }

        // Let SMultiLineEditableText::OnKeyChar handle it.
        return FReply::Unhandled();
    }

    TOptional<int32> SShaderEditorBox::FindFoldMarker(int32 InIndex) const
    {
        for (int32 i = 0; i < VisibleFoldMarkers.Num(); i++)
        {
            if (VisibleFoldMarkers[i].RelativeLineIndex == InIndex)
            {
                return i;
            }
        }
        return {};
    }

    FString SShaderEditorBox::UnFoldText(const FTextSelection& DisplayedTextRange)
    {
        FString TextAfterUnfolding;
        if(VisibleFoldMarkers.IsEmpty())
        {
            ShaderMarshaller->TextLayout->GetSelectionAsText(TextAfterUnfolding, DisplayedTextRange);
            return TextAfterUnfolding;
        }
        
        const TArray< FTextLayout::FLineModel >& Lines = ShaderMarshaller->TextLayout->GetLineModels();
        int32 StartLineIndex = DisplayedTextRange.GetBeginning().GetLineIndex();
        int32 EndLineIndex = DisplayedTextRange.GetEnd().GetLineIndex();
        int32 StartOffset = DisplayedTextRange.GetBeginning().GetOffset();
        int32 EndOffset = DisplayedTextRange.GetEnd().GetOffset();

        for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
        {
            FString SelectedText;
            int32 TextStart{};
            int32 TextEnd{};
            if(LineIndex == StartLineIndex && LineIndex == EndLineIndex)
            {
                ShaderMarshaller->TextLayout->GetSelectionAsText(SelectedText, DisplayedTextRange);
                TextStart = StartOffset;
                TextEnd = EndOffset;
            }
            else if (LineIndex == StartLineIndex)
            {
                SelectedText = Lines[LineIndex].Text->Mid(StartOffset);
                TextStart = StartOffset;
                TextEnd = Lines[LineIndex].Text->Len();
            }
            else if(LineIndex == EndLineIndex)
            {
                SelectedText = Lines[LineIndex].Text->Mid(0, EndOffset);
                TextStart = 0;
                TextEnd = EndOffset;
            }
            else
            {
                SelectedText = *Lines[LineIndex].Text;
                TextStart = 0;
                TextEnd = Lines[LineIndex].Text->Len();
            }

            TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
            if (MarkerIndex)
            {
                const auto& Marker = VisibleFoldMarkers[*MarkerIndex];
                if(Marker.Offset >= TextStart && Marker.Offset < TextEnd)
                {
                    int32 OffsetForSelectedText = Marker.Offset - TextStart;
                    TextAfterUnfolding += SelectedText.Mid(0, OffsetForSelectedText);
                    TextAfterUnfolding += Marker.GetTotalFoldedLineTexts();
                    TextAfterUnfolding += SelectedText.Mid(OffsetForSelectedText + 1);
                }
                else
                {
                    TextAfterUnfolding += *SelectedText;
                }
            }
            else
            {
                TextAfterUnfolding += *SelectedText;
            }

            if (LineIndex < EndLineIndex) {
                TextAfterUnfolding += "\n";
            }
        }

        return TextAfterUnfolding;
    }

    void SShaderEditorBox::RemoveFoldMarker(int32 InIndex)
    {
        for (int32 i = 0; i < VisibleFoldMarkers.Num(); i++)
        {
            if (VisibleFoldMarkers[i].RelativeLineIndex == InIndex)
            {
                VisibleFoldMarkers.RemoveAt(i);
                return;
            }
        }
    }

    FReply SShaderEditorBox::OnFold(int32 LineNumber)
    {
        int32 LineIndex = GetLineIndex(LineNumber);
        FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
        TArray< FTextLayout::FLineModel >& Lines = const_cast<TArray<FTextLayout::FLineModel>&>(ShaderTextLayout->GetLineModels());
        
		IsFoldEditTransaction = true;
		ShaderMultiLineEditableTextLayout->BeginEditTransation();
		
        TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
        
        const FTextLocation& CurCursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
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
                    FoldMarker& FoldedMarker = VisibleFoldMarkers[*MarkerIndex];
                    FoldMarker ChildMarker = FoldedMarker;
                    ChildMarker.RelativeLineIndex = FoldedMarker.RelativeLineIndex - Marker.RelativeLineIndex;
                    
                    Marker.ChildFoldMarkers.Add(MoveTemp(ChildMarker));
                    RemoveFoldMarker(LineIndex);
                }
            }
            VisibleFoldMarkers.Add(MoveTemp(Marker));
            VisibleFoldMarkers.Sort([](const FoldMarker& A, const FoldMarker& B) { return A.RelativeLineIndex < B.RelativeLineIndex; });
            
            //The cursor is within the fold now.
            if (CurCursorLocation.GetLineIndex() >= FoldedBeginningRow && CurCursorLocation.GetLineIndex() <= FoldedEndRow)
            {
                const FTextLocation NewCursorLocation(FoldedBeginningRow, BeginLine.Text->Len());
                ShaderMultiLineEditableText->GoTo(NewCursorLocation);
            }
            else if(CurCursorLocation.GetLineIndex() > FoldedEndRow)
            {
                int32 FoldedLineNum = FoldedEndRow - FoldedBeginningRow;
                const FTextLocation NewCursorLocation(CurCursorLocation.GetLineIndex() - FoldedLineNum, CurCursorLocation.GetOffset());
                ShaderMultiLineEditableText->GoTo(NewCursorLocation);
            }
        }
        //UnFold
        else
        {
            const FoldMarker* Marker = &VisibleFoldMarkers[*MarkerIndex];
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
                auto NewMarker = Marker->ChildFoldMarkers[i];
                NewMarker.RelativeLineIndex += Marker->RelativeLineIndex;
                VisibleFoldMarkers.Add(MoveTemp(NewMarker));
            }

            RemoveFoldMarker(LineIndex);
            VisibleFoldMarkers.Sort([](const FoldMarker& A, const FoldMarker& B) { return A.RelativeLineIndex < B.RelativeLineIndex; });
            
            if(CurCursorLocation.GetLineIndex() > LineIndex)
            {
                const FTextLocation NewCursorLocation(CurCursorLocation.GetLineIndex() + FoldedLineText.Num() - 1, CurCursorLocation.GetOffset());
                ShaderMultiLineEditableText->GoTo(NewCursorLocation);
            }
        }
		
		ShaderMultiLineEditableTextLayout->EndEditTransaction();
		IsFoldEditTransaction = false;
		
        return FReply::Handled();
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
        int32 LineNumber = FCString::Atoi(*(*Item).ToString());
        const int32 LineIndex = GetLineIndex(LineNumber);
		
		float MinWidth = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("0"), CodeFontInfo).X;
		MinWidth *= FString::FromInt(MaxLineNumber).Len();
        
        TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
            .Font(CodeFontInfo)
            .ColorAndOpacity_Lambda([this, LineIndex]{
                const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
                const int32 BeginLineIndex = Selection.GetBeginning().GetLineIndex();
                const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
                if(LineIndex >= BeginLineIndex && LineIndex <= EndLineIndex)
                {
                    return HighlightLineNumberTextColor;
                }
                return NormalLineNumberTextColor;
            })
            .Text(*Item)
            .Justification(ETextJustify::Right)
            .MinDesiredWidth(MinWidth);
		
		LineNumberTextBlock->SetOnMouseButtonDown(FPointerEventHandler::CreateLambda([this, LineNumber](const FGeometry&, const FPointerEvent&){
			if(BreakPointLines.Contains(LineNumber))
			{
				BreakPointLines.Remove(LineNumber);
			}
			else{
				BreakPointLines.Add(LineNumber);
			}
			return FReply::Handled();
		}));

        TSharedPtr<SButton> FoldingArrow = SNew(SButton)
            .ContentPadding(FMargin(0, 0))
            .ButtonColorAndOpacity_Lambda([this, LineIndex] {
                if(ShaderMarshaller->FoldingBraceGroups.Contains(LineIndex))
                {
                    return FLinearColor{1, 1, 1, FoldingArrowAnim.GetLerp()};
                }
                return FLinearColor::White;
            })
            .Visibility_Lambda([this, LineIndex]{
                if(ShaderMarshaller->FoldingBraceGroups.Contains(LineIndex) || FindFoldMarker(LineIndex))
                {
                    return EVisibility::Visible;
                }
                return EVisibility::Hidden;
            })
            .IsFocusable(false)
            .OnClicked(this, &SShaderEditorBox::OnFold, LineNumber);
        
        FoldingArrow->SetPadding(FMargin{});
        if(FindFoldMarker(LineIndex))
        {
            FoldingArrow->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowRightButton"));
        }
		else
		{
			FoldingArrow->SetButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FButtonStyle>("ArrowDownButton"));
		}
        
        auto ItemErrorMarker = SNew(STextBlock)
        .Font(CodeFontInfo)
        .ColorAndOpacity(FLinearColor::Red)
        .Visibility_Lambda([this, Item]{
            const int32 CurLineNumber = FCString::Atoi(*Item->ToString());
            const int32 LineIndex = GetLineIndex(CurLineNumber);
            if(EffectMarshller->LineNumberToErrorInfo.Contains(CurLineNumber))
            {
                return EVisibility::Visible;
            }
            else if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
            {
                int32 FoldedLineCounts = VisibleFoldMarkers[*MarkerIndex].GetFoldedLineCounts();
                for (int32 i = CurLineNumber; i <= CurLineNumber + FoldedLineCounts + 1; i++)
                {
                    if (EffectMarshller->LineNumberToErrorInfo.Contains(i))
                    {
                        return EVisibility::Visible;
                    }
                }
            }
            return EVisibility::Hidden;
        })
        .Text(FText::FromString(ErrorMarkerText));
		
		auto BreakPoint = SNew(SImage)
		.Image(FAppStyle::Get().GetBrush("Icons.BulletPoint"))
		.ColorAndOpacity(FLinearColor::Red)
		.Visibility_Lambda([this, LineNumber, ItemErrorMarker]{
			if(BreakPointLines.Contains(LineNumber) && ItemErrorMarker->GetVisibility() != EVisibility::Visible)
			{
				return EVisibility::HitTestInvisible;
			}
			return EVisibility::Hidden;
		});
		
		auto LineNumberRow = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
			.Content()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							ItemErrorMarker
						]
						+SOverlay::Slot()
						[
							BreakPoint
						]
					]
					+ SHorizontalBox::Slot()
					[
						LineNumberTextBlock.ToSharedRef()
					]
					+ SHorizontalBox::Slot()
					.Padding(5, 0, 5, 0)
					.AutoWidth()
					[
						SNew(SScaleBox)
						[
							FoldingArrow.ToSharedRef()
						]
					]
				]
				+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(1.0)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity(FLinearColor{1,0,0,0.7f})
					]
				]
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.Image(FAppStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor{1,0,0,0.11f})
				]
			];
        
		return LineNumberRow;
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
        //DummyTextBlock is used to keep the same layout as LineNumber and MultiLineEditableText.
        TSharedPtr<STextBlock> DummyTextBlock = SNew(STextBlock)
            .Font(CodeFontInfo)
            .Visibility(EVisibility::Hidden);
		int32 LineNumber = FCString::Atoi(*(*Item).ToString());
		
        TSharedPtr<STableRow<LineNumberItemPtr>> LineTip = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
            .Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineTipItemStyle"))
            .Content()
            [
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					DummyTextBlock.ToSharedRef()
				]
				+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(1.0)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity(FLinearColor{1,0,0,0.7f})
					]
				]
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect"))
					.ColorAndOpacity(FLinearColor{1,0,0,0.7f})
				]
            ];

    
        LineTip->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, LineNumber]{
            const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
            const int32 CurLineIndex = CursorLocation.GetLineIndex();
            auto FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
            if(FocusedWidget == ShaderMultiLineEditableText && LineNumber == GetLineNumber(CurLineIndex) && !BreakPointLines.Contains(LineNumber))
            {
                double CurTime = FPlatformTime::Seconds();
                float Speed = 2.0f;
                double AnimatedOpacity = (HighlightLineTipColor.A - 0.1f) * FMath::Pow(FMath::Abs(FMath::Sin(CurTime * Speed)),1.8) + 0.1f;
                return FLinearColor{ HighlightLineTipColor.R, HighlightLineTipColor.G, HighlightLineTipColor.B, (float)AnimatedOpacity };
            }
            return NormalLineTipColor;
        }));
        
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
		TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::String, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorStringText"));
        TokenStyleMap.Add(HlslHighLightTokenizer::TokenType::Other, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText"));
    }

    void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
    {
        TextLayout = &TargetTextLayout;
		OwnerWidget->CurrentEditorSource = SourceString;

        TArray<HlslHighLightTokenizer::BraceGroup> BraceGroups;
        TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines = Tokenizer->Tokenize(SourceString, BraceGroups);

        FoldingBraceGroups.Empty();
        for (const auto& BraceGroup : BraceGroups)
        {
            if (BraceGroup.LeftBracePos.Row != BraceGroup.RightBracePos.Row && !FoldingBraceGroups.Contains(BraceGroup.LeftBracePos.Row)) {
                FoldingBraceGroups.Add(BraceGroup.LeftBracePos.Row, BraceGroup);
            }
        }
        FSlateEditableTextLayout* EditableTextLayout = OwnerWidget->ShaderMultiLineEditableTextLayout;
        if(EditableTextLayout && EditableTextLayout->CurrentUndoLevel >= 0)
        {
            ShaderUndoState* UndoState = static_cast<ShaderUndoState*>(EditableTextLayout->UndoStates[EditableTextLayout->CurrentUndoLevel].Get());
            OwnerWidget->VisibleFoldMarkers = UndoState->FoldMarkers;
			OwnerWidget->BreakPointLines = UndoState->BreakPointLines;
        }
        else
        {
            TArray<FoldMarker> NewFoldMarkers;
            for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); LineIndex++)
            {
                TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(TokenizedLines[LineIndex].LineRange.BeginIndex, TokenizedLines[LineIndex].LineRange.Len()));
                int32 MarkerOffset = LineText->Find(FoldMarkerText);
                if (MarkerOffset != INDEX_NONE)
                {
                    NewFoldMarkers.Add({ LineIndex, MarkerOffset});
                }
            }

            check(NewFoldMarkers.Num() == OwnerWidget->VisibleFoldMarkers.Num());
            
            for (int32 i = 0; i < OwnerWidget->VisibleFoldMarkers.Num(); i++)
            {
                auto& Marker = OwnerWidget->VisibleFoldMarkers[i];
                Marker.RelativeLineIndex = NewFoldMarkers[i].RelativeLineIndex;
                Marker.Offset = NewFoldMarkers[i].Offset;
            }
        }

        TArray<FTextLineHighlight> Highlights;
        for (int32 i = 0; i < OwnerWidget->VisibleFoldMarkers.Num(); i++)
        {
            auto& Marker = OwnerWidget->VisibleFoldMarkers[i];
            Highlights.Emplace(Marker.RelativeLineIndex, FTextRange{ Marker.Offset, Marker.Offset + 1 }, -1, FoldMarkerHighLighter::Create());
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

        FTextSelection UnFoldingRange = FTextSelection{ {0, 0}, {LinesToAdd.Num() - 1, LinesToAdd[LinesToAdd.Num() - 1].Text->Len()} };
        FString TextAfterUnfolding = OwnerWidget->UnFoldText(MoveTemp(UnFoldingRange));
		OwnerWidget->OnShaderTextChanged(TextAfterUnfolding);
        OwnerWidget->UpdateLineNumberData();
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
