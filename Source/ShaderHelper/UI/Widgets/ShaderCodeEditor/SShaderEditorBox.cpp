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
#include <Widgets/Text/SlateTextBlockLayout.h>
#include <Framework/Text/SlateTextHighlightRunRenderer.h>
#include <Fonts/FontMeasure.h>
#include <Styling/StyleColors.h>
#include "GpuApi/Spirv/SpirvExpressionVM.h"

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<FUICommandList>, UICommandList)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>, CursorLineHighlighter)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, SlateEditableTextTypes::FCursorInfo, CursorInfo)
STEAL_PRIVATE_MEMBER(STextBlock, TUniquePtr< FSlateTextBlockLayout >, TextLayoutCache)
STEAL_PRIVATE_MEMBER(FSlateTextBlockLayout, TSharedPtr<FSlateTextLayout>, TextLayout)
STEAL_PRIVATE_MEMBER(FTextLayout, uint8, DirtyFlags)
CALL_PRIVATE_FUNCTION(SMultiLineEditableText_OnMouseWheel, SMultiLineEditableText, OnMouseWheel,, FReply, const FGeometry&, const FPointerEvent&)

using namespace FW;

namespace SH
{

const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
const FLinearColor HighlightLineNumberTextColor = { 0.7f,0.7f,0.7f,0.9f };

const FLinearColor NormalLineTipColor = { 1.0f,1.0f,1.0f,0.0f };
const FLinearColor HighlightLineTipColor = { 1.0f,1.0f,1.0f,0.2f };

const FString FoldMarkerText = TEXT("⇿");

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
        TArray<HlslTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString);
        const HlslTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
        for(const auto& Token : TokenLine.Tokens)
        {
            if(InIndex > Token.BeginOffset && InIndex <= Token.EndOffset)
            {
                CurrentPosition = Token.BeginOffset;
                break;
            }
        }
        return CurrentPosition >= InIndex ? INDEX_NONE : CurrentPosition;
    }

    int32 TokenBreakIterator::MoveToCandidateAfter(const int32 InIndex)
    {
        TArray<HlslTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString);
        const HlslTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
        for(const auto& Token : TokenLine.Tokens)
        {
            if(InIndex >= Token.BeginOffset && InIndex < Token.EndOffset)
            {
                CurrentPosition = Token.EndOffset;
                break;
            }
        }
        return CurrentPosition <= InIndex ? INDEX_NONE : CurrentPosition;
    }
    
    SShaderEditorBox::~SShaderEditorBox()
    {
		ShaderAssetObj->OnRefreshBuilder.RemoveAll(this);
		
        bQuitISense = true;
        ISenseEvent->Trigger();
        ISenseThread->Join();
        FPlatformProcess::ReturnSynchEventToPool(ISenseEvent);
		
		bQuitISyntax = true;
		SyntaxEvent->Trigger();
		SyntaxThread->Join();
		FPlatformProcess::ReturnSynchEventToPool(SyntaxEvent);
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

	FSlateFontInfo& SShaderEditorBox::GetCodeFontInfo()
	{
		static FSlateFontInfo CodeFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		return CodeFontInfo;
	}

	FTextBlockStyle& SShaderEditorBox::GetTokenStyle(HLSL::TokenType InType)
	{
		static TMap<HLSL::TokenType, FTextBlockStyle> TokenStyleMap {
			{ HLSL::TokenType::Number, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNumberText") },
			{ HLSL::TokenType::Keyword, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorKeywordText") },
			{ HLSL::TokenType::Punctuation, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText") },
			{ HLSL::TokenType::BuildtinFunc, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorBuildtinFuncText") },
			{ HLSL::TokenType::BuildtinType, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorBuildtinTypeText") },
			{ HLSL::TokenType::Identifier, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText") },
			{ HLSL::TokenType::Preprocess, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorPreprocessText") },
			{ HLSL::TokenType::Comment, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorCommentText") },
			{ HLSL::TokenType::String, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorStringText") },
			{ HLSL::TokenType::Other, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText") },
			
			{ HLSL::TokenType::Func, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorFuncText")},
			{ HLSL::TokenType::Type, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorTypeText")},
			{ HLSL::TokenType::Parm, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorParmText")},
			{ HLSL::TokenType::Var, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorVarText")},
			{ HLSL::TokenType::LocalVar, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorNormalText")},
		};
		auto& CodeFontInfo = GetCodeFontInfo();
		for(auto& [_, Style] : TokenStyleMap)
		{
			if(Style.Font != CodeFontInfo)
			{
				Style.SetFont(CodeFontInfo);
			}
		}
		return TokenStyleMap[InType];
	}

    void SShaderEditorBox::Construct(const FArguments& InArgs)
    {
		ShaderAssetObj = InArgs._ShaderAssetObj;
		ShaderAssetObj->OnRefreshBuilder.AddLambda([this]{
			ISenseTask Task{};
			Task.ShaderDesc = ShaderAssetObj->GetShaderDesc(CurrentShaderSource);
			ISenseQueue.Enqueue(MoveTemp(Task));
			ISenseEvent->Trigger();
		});
		
        SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical).Padding(0)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar")).Thickness(8.0f);
		ShaderMultiLineVScrollBar->OnSetState = [this](float InOffsetFraction, float InThumbSizeFraction) {
			LineNumberList->SetScrollOffset(InOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
			LineTipList->SetScrollOffset(InOffsetFraction * LineTipList->GetNumItemsBeingObserved());
		};
        SAssignNew(ShaderMultiLineHScrollBar, SScrollBar).Orientation(EOrientation::Orient_Horizontal).Padding(0)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar")).Thickness(8.0f);

        ShaderMarshaller = MakeShared<FShaderEditorMarshaller>(this, MakeShared<HlslTokenizer>());
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
					ShaderTU TU{Shader->GetProcessedSourceText(), Shader->GetIncludeDirs()};
					DiagnosticInfos = TU.GetDiagnostic();
					
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
                    
					if(ISenseQueue.IsEmpty())
					{
						bRefreshIsense.store(true, std::memory_order_release);
					}
                }

				if(ISenseQueue.IsEmpty())
				{
					ISenseEvent->Wait();
				}
            }
        });
		
		SyntaxEvent = FPlatformProcess::GetSynchEventFromPool();
		SyntaxThread = MakeUnique<FThread>(TEXT("SyntaxThread"), [this]{
			while(!bQuitISyntax)
			{
				SyntaxTask Task;
				while(SyntaxQueue.Dequeue(Task));
				if(Task.ShaderDesc)
				{
					TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(Task.ShaderDesc.value());
					ShaderTU TU{Shader->GetSourceText(), Shader->GetIncludeDirs()};
					LineSyntaxHighlightMaps.Reset();
					LineSyntaxHighlightMaps.SetNum(Task.LineTokens.Num());
					int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
					for(int LineIndex = 0; LineIndex < Task.LineTokens.Num(); LineIndex++)
					{
						for (const HlslTokenizer::Token& Token : Task.LineTokens[LineIndex])
						{
							HLSL::TokenType NewTokenType = TU.GetTokenType(Token.Type, ExtraLineNum + LineIndex + 1, Token.BeginOffset + 1);
							if(NewTokenType != Token.Type)
							{
								LineSyntaxHighlightMaps[LineIndex].Add(FTextRange{Token.BeginOffset, Token.EndOffset},
																	  &GetTokenStyle(NewTokenType));
							}
						}
					}
					
					FuncScopes = TU.GetFuncScopes();
					
					if(SyntaxQueue.IsEmpty())
					{
						bRefreshSyntax.store(true, std::memory_order_release);
					}
				}
				
				if(SyntaxQueue.IsEmpty())
				{
					SyntaxEvent->Wait();
				}
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
						.IsSelfDisabled(true)
						.IsFocusable(false)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SAssignNew(ShaderMultiLineEditableText, SMultiLineEditableText)
							.Text(InitialShaderText)
							.EnableUniformFont(true)
							.Font(GetCodeFontInfo())
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
									bTryMergeUndoState = false;
								}
								bKeyChar = false;
							})
						]
						+ SOverlay::Slot()
						[
							SAssignNew(EffectMultiLineEditableText, SMultiLineEditableText)
							.IsReadOnly(true)
							.Font(GetCodeFontInfo())
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
						.VAlign(VAlign_Bottom)
						[
							ShaderMultiLineHScrollBar.ToSharedRef()
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
								if(TipPos.X + 240 > Area.X)
								{
									TipPos.X -= 240;
								}
								return TipPos;
							})
							[
								SNew(SBorder)
								.Visibility_Lambda([this]{
									return bTryComplete? EVisibility::Visible : EVisibility::Collapsed;
								})
								.BorderBackgroundColor(FLinearColor{1,1,1,0.8f})
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
										.IsFocusable(false)
										.OnMouseButtonClick_Lambda([this](CandidateItemPtr ClickedItem){
											InsertCompletionText(ClickedItem);
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
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						ShaderMultiLineVScrollBar.ToSharedRef()
					]
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildInfoBar()
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
		
		ShaderMultiLineEditableTextLayout->TryPushUndoState = [this](SlateEditableTextTypes::FUndoState* InUndoState){
			if(!ShaderMultiLineEditableTextLayout->UndoStates.IsEmpty() && bTryMergeUndoState)
			{
				//Merge some undo states
				SlateEditableTextTypes::FUndoState* LastState = ShaderMultiLineEditableTextLayout->UndoStates.Last().Get();
				if(LastState->Commands.Num() == 1 && InUndoState->Commands.Num() == 1)
				{
					TSharedPtr<SlateEditableTextTypes::FEditCommand> LastCmd = LastState->Commands[0];
					TSharedPtr<SlateEditableTextTypes::FEditCommand> CurCmd = InUndoState->Commands[0];
					if(LastCmd->GetType() == SlateEditableTextTypes::FEditCommand::Type::InsertAt
					   && CurCmd->GetType() == SlateEditableTextTypes::FEditCommand::Type::InsertAt)
					{
						TSharedPtr<FInsertAtCmd> LastInsertCmd = StaticCastSharedPtr<FInsertAtCmd>(LastCmd);
						TSharedPtr<FInsertAtCmd> CurInsertCmd = StaticCastSharedPtr<FInsertAtCmd>(CurCmd);
						auto IsIdentifierStr = [](const FString& Str){
                            for(auto C : Str)
                                if(!FChar::IsIdentifier(C)) return false;
                            return true;
                        };
                        auto IsWhitespaceStr = [](const FString& Str){
                            for(auto C : Str)
                                if(!FChar::IsWhitespace(C)) return false;
                            return true;
                        };

                        const FString& LastText = LastInsertCmd->GetText();
                        const FString& CurText = CurInsertCmd->GetText();

                        bool bBothIdentifier = IsIdentifierStr(LastText) && IsIdentifierStr(CurText);
                        bool bBothWhitespace = IsWhitespaceStr(LastText) && IsWhitespaceStr(CurText);

                        if(LastInsertCmd->GetLocation().GetLineIndex() == CurInsertCmd->GetLocation().GetLineIndex()
                        && LastInsertCmd->GetLocation().GetOffset() + LastInsertCmd->GetText().Len() == CurInsertCmd->GetLocation().GetOffset()
                        && CurInsertCmd->GetText().Len() == 1
                        && (bBothIdentifier || bBothWhitespace))
                        {
                            auto NewInsertCmd = MakeShared<FInsertAtCmd>(LastInsertCmd->GetLocation(), LastInsertCmd->GetText() + CurInsertCmd->GetText());
                            LastState->Commands[0] = MoveTemp(NewInsertCmd);
                            return false;
                        }
					}
				}
				
			}
			return true;
		};
        
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
		case EditState::Editing:   return LOCALIZATION("Editing");
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
		case EditState::Editing:   return FLinearColor::Gray;
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
        return LineNumberToIndexMap.Contains(InLineNumber) ? LineNumberToIndexMap[InLineNumber] : INDEX_NONE;
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
							auto& CodeFontInfo = GetCodeFontInfo();
                            CodeFontInfo.Size = i;
                            ShaderMarshaller->MakeDirty();
							ShaderMultiLineEditableText->SetFont(CodeFontInfo);
                            ShaderMultiLineEditableText->Refresh();
                            EffectMarshller->MakeDirty();
							EffectMultiLineEditableText->SetFont(CodeFontInfo);
                            EffectMultiLineEditableText->Refresh();
                        }),
                        FCanExecuteAction(),
                        FIsActionChecked::CreateLambda(
                        [this, i]()
                        {
                            return GetCodeFontInfo().Size == i;
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
			.Padding(2,0,0,0)
            .AutoWidth()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(this, &SShaderEditorBox::GetEditStateColor)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.MinDesiredWidth(80)
                    .Font(InforBarFontInfo)
                    .ColorAndOpacity(FLinearColor::Black)
                    .Text(this, &SShaderEditorBox::GetEditStateText)
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
		//TODO:Tab character?
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
		
        LineNumberToIndexMap.Reset();
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
            LineNumberToIndexMap.Add(LineNumber, LineIndex);
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
        return CallPrivate_SMultiLineEditableText_OnMouseWheel(*ShaderMultiLineEditableText, ShaderMultiLineEditableText->GetTickSpaceGeometry(), MouseEvent);
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

    void SShaderEditorBox::UpdateEffectText()
    {
        //Sync the effect text location when scrolling ShaderMultiLineEditableText.
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

	void SShaderEditorBox::RefreshSyntaxHighlight()
	{
		LineSyntaxHighlightMapsCopy = LineSyntaxHighlightMaps;
		for(int LineIndex = 0; LineIndex < LineSyntaxHighlightMapsCopy.Num(); LineIndex++)
		{
			auto& SyntaxHighlightMap = LineSyntaxHighlightMapsCopy[LineIndex];
			auto& LineModel =  ShaderMarshaller->TextLayout->GetLineModels()[LineIndex];
			for(const auto& [TokenRange, Style] : SyntaxHighlightMap)
			{
				for(auto& RunModel : LineModel.Runs)
				{
					TSharedRef<IRun> Run = RunModel.GetRun();
					if(Run->GetTextRange() == TokenRange)
					{
						RunModel = FTextLayout::FRunModel(FSlateTextStyleRefRun::Create(FRunInfo(), LineModel.Text, *Style, TokenRange));
					}
				}
			}
		}
		
		GetPrivate_FTextLayout_DirtyFlags(*ShaderMarshaller->TextLayout) |= (1 << 0);
	}

    void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
		//ShaderMultiLineEditableText updates its VScrollBar when it ticks, and we sync the LineNumberList/LineTipList according to its VScrollBar callback.
		//Which leads to a delay of one frame in the drawing of the list, so here we tick it earlier.
		ShaderMultiLineEditableTextLayout->Tick(ShaderMultiLineEditableText->GetTickSpaceGeometry(), InCurrentTime, InDeltaTime);
        
        if(bRefreshIsense.load(std::memory_order_acquire))
        {
            RefreshLineNumberToDiagInfo();
            RefreshCodeCompletionTip();
            bRefreshIsense.store(false, std::memory_order_relaxed);
        }
		
		if(bRefreshSyntax.load(std::memory_order_acquire))
		{
			RefreshSyntaxHighlight();
			FuncScopesCopy = FuncScopes;
			bRefreshSyntax.store(false, std::memory_order_relaxed);
		}
        
        UpdateEffectText();
    }

    void SShaderEditorBox::OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
    {
        bTryComplete = false;
		bTryMergeUndoState = false;
    }

    FReply SShaderEditorBox::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
    {
        return FReply::Handled().SetUserFocus(ShaderMultiLineEditableText.ToSharedRef());
    }

    void SShaderEditorBox::InsertCompletionText(CandidateItemPtr InItem)
    {
		SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
		
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
		
		//TextLayout->RemoveAt does not handle the cursor, so manually call the goto function.
		//However, directly call SelectText and DeleteSelectedText also can do it.
		ShaderTextLayout->RemoveAt({CursorRow, InsertCol}, CurToken.Len());
        ShaderMultiLineEditableText->GoTo(NewCursorLocation);
		
		FString InsertedText = InItem->Text;
		if(InItem->Kind == HLSL::CandidateKind::Func)
		{
			InsertedText += "()";
			ShaderMultiLineEditableText->InsertTextAtCursor(MoveTemp(InsertedText));
			const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
			ShaderMultiLineEditableText->GoTo({CursorLocation.GetLineIndex(), CursorLocation.GetOffset() - 1});
		}
		else
		{
			ShaderMultiLineEditableText->InsertTextAtCursor(MoveTemp(InsertedText));
		}
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateCodeCompletionItem(CandidateItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
		auto CandidateText = SNew(STextBlock).Font(GetCodeFontInfo())
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
		
        auto Row = SNew(STableRow<CandidateItemPtr>, OwnerTable)
		.MouseButtonDownFocusWidget(ShaderMultiLineEditableText)
		.DetectDrag(false)
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
		Row->SetBorderBackgroundColor(FLinearColor{1,1,1,0.5f});
		return Row;
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

    void SShaderEditorBox::RefreshLineNumberToDiagInfo()
    {
        EffectMarshller->LineNumberToDiagInfo.Reset();
        for (const ShaderDiagnosticInfo& DiagInfo : DiagnosticInfos)
        {
            int32 DiagInfoLineNumber = DiagInfo.Row - ShaderAssetObj->GetExtraLineNum();
            if (!EffectMarshller->LineNumberToDiagInfo.Contains(DiagInfoLineNumber))
            {
                int32 LineIndex = GetLineIndex(DiagInfoLineNumber);
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

				FString DisplayInfo = DummyText + TEXT("  ");
				if(!DiagInfo.Error.IsEmpty())
				{
					DisplayInfo += DiagInfo.Error;
				}
				else if(!DiagInfo.Warn.IsEmpty())
				{
					DisplayInfo += DiagInfo.Warn;
				}
                FTextRange DummyRange{ 0, DummyText.Len() };
                FTextRange InfoRange{ DummyText.Len(), DisplayInfo.Len() };

				EffectMarshller->LineNumberToDiagInfo.Add(DiagInfoLineNumber, {
					.IsError = !DiagInfo.Error.IsEmpty(),
					.IsWarn = !DiagInfo.Warn.IsEmpty(),
					.DummyRange = MoveTemp(DummyRange),
					.InfoRange = MoveTemp(InfoRange),
					.TotalInfo = MoveTemp(DisplayInfo)
				});
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
		int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();
		
		//TODO Async and show "Compiling" state
        TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(ShaderAssetObj->GetShaderDesc(CurrentShaderSource));
		FString ErrorInfo, WarnInfo;
        if (GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo))
        {
			ShaderAssetObj->Shader = Shader;
			ShaderAssetObj->bCompilationSucceed = true;
            CurEditState = EditState::Succeed;
			
			if(!WarnInfo.IsEmpty())
			{
				WarnInfo = AdjustDiagLineNumber(WarnInfo, -AddedLineNum);
				SH_LOG(LogShader, Warning, TEXT("%s"), *WarnInfo);
			}
        }
		
		if(!ErrorInfo.IsEmpty())
        {
			ErrorInfo = AdjustDiagLineNumber(ErrorInfo, -AddedLineNum);
			SH_LOG(LogShader, Error, TEXT("%s"), *ErrorInfo);
			ShaderAssetObj->bCompilationSucceed = false;
            CurEditState = EditState::Failed;
        }
    }

    void SShaderEditorBox::OnShaderTextChanged(const FString& InShaderSouce)
    {
		CurEditState = EditState::Editing;
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
            
            TArray<HlslTokenizer::TokenizedLine> TokenizedLines = ShaderMarshaller->Tokenizer->Tokenize(CursorLeft, true);
			auto Token = TokenizedLines[0].Tokens.Last();
            FString LeftToken = CursorLeft.Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
            if(TokenizedLines[0].Tokens.Num() > 1)
            {
				Token = TokenizedLines[0].Tokens.Last(1);
                FString LeftToken2 = CursorLeft.Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
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
		
		const FTextSelection& Selection = ShaderMultiLineEditableText->GetSelection();
		const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		
		const FTextLocation& CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
		const int32 CursorLine = CursorLocation.GetLineIndex();
		const int32 CursorOffset = CursorLocation.GetOffset();
		
		if(Key == EKeys::K && InKeyEvent.GetModifierKeys().IsControlDown())
		{
			bTryToggleCommentSelection = true;
		}
		else if(Key == EKeys::C && bTryToggleCommentSelection)
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
			int32 FinalInserCol = std::numeric_limits<int32>::max();
			TMap<int32, bool> CanInsert;
			for(int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
			{
				FString LineText;
				ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);
				CanInsert.Add(LineIndex, false);
				
				int32 InsertCol = 0;
				do
				{
					if(InsertCol >= LineText.Len())
					{
						break;
					}
					if(!FChar::IsWhitespace(LineText[InsertCol]))
					{
						CanInsert[LineIndex] = true;
						break;
					}
					InsertCol++;
				}while(InsertCol);

				if(CanInsert[LineIndex]) {
					FinalInserCol = FMath::Min(FinalInserCol, InsertCol);
				}
			}
			FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
			if(FinalInserCol != std::numeric_limits<int32>::max())
			{
                FTextLocation OldStart = Selection.GetBeginning();
                FTextLocation OldEnd = Selection.GetEnd();
                bool bCursorAtEnd = (OldEnd == CursorLocation);

				for(int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
				{
					if(CanInsert[LineIndex])
					{
						ShaderTextLayout->InsertAt(FTextLocation{ LineIndex, FinalInserCol }, "// ");
                        if(OldStart.GetLineIndex() == LineIndex && OldStart.GetOffset() >= FinalInserCol)
                            OldStart = FTextLocation(LineIndex, OldStart.GetOffset() + 3);
                        if(OldEnd.GetLineIndex() == LineIndex && OldEnd.GetOffset() >= FinalInserCol)
                            OldEnd = FTextLocation(LineIndex, OldEnd.GetOffset() + 3);
					}
				}
                if(bCursorAtEnd)
                {
                    ShaderMultiLineEditableText->SelectText(OldStart, OldEnd);
                }
                else
                {
                    ShaderMultiLineEditableText->SelectText(OldEnd, OldStart);
                }
			}
			bTryToggleCommentSelection = false;
		}
		else if(Key == EKeys::U && bTryToggleCommentSelection)
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
			
			FTextLocation OldStart = Selection.GetBeginning();
			FTextLocation OldEnd = Selection.GetEnd();
			bool bCursorAtEnd = (OldEnd == CursorLocation);
			
			for(int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
			{
				FString LineText;
				ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);

				int32 RemoveCol = 0;
				while(RemoveCol < LineText.Len() && FChar::IsWhitespace(LineText[RemoveCol]))
				{
					++RemoveCol;
				}

				if(LineText.Mid(RemoveCol, 2) == TEXT("//"))
				{
					int32 RemoveLen = 2;
					if(LineText.Len() > RemoveCol + 2 && LineText[RemoveCol + 2] == TEXT(' '))
					{
						++RemoveLen;
					}
					FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
					ShaderTextLayout->RemoveAt(FTextLocation{LineIndex, RemoveCol}, RemoveLen);
					
					if(OldStart.GetLineIndex() == LineIndex && OldStart.GetOffset() >= RemoveCol)
						  OldStart = FTextLocation(OldStart, -FMath::Min(RemoveLen, OldStart.GetOffset() - RemoveCol));
					if(OldEnd.GetLineIndex() == LineIndex && OldEnd.GetOffset() >= RemoveCol)
						  OldEnd = FTextLocation(OldEnd, -FMath::Min(RemoveLen, OldEnd.GetOffset()  - RemoveCol));
				}
			}
			if(bCursorAtEnd)
			{
				ShaderMultiLineEditableText->SelectText(OldStart, OldEnd);
			}
			else
			{
				ShaderMultiLineEditableText->SelectText(OldEnd, OldStart);
			}
			bTryToggleCommentSelection = false;
			
		}
		else
		{
			bTryToggleCommentSelection = false;
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
				//HandleKeyChar will handle it, no need to dispatch to SMultiLineEditableText::OnKeyDown
                return FReply::Handled();
            }
        }
		else
		{
			if(Key == EKeys::Enter)
			{
				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
				ShaderMultiLineEditableTextLayout->HandleCarriageReturn(InKeyEvent.IsRepeat());
				// at this point, the text after the text cursor is already in a new line
				HandleAutoIndent();
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
		bKeyChar = true;
        if (Character == TEXT('\b'))
        {
            if (!Text->AnyTextSelected())
            {
                // if we are deleting a single open brace
                // look for a matching close brace following it and delete the close brace as well
                const FTextLocation& CursorLocation = Text->GetCursorLocation();
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
                const FTextSelection& Selection = Text->GetSelection();
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
                InsertCompletionText(CurSelectedCandidate);
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
                InsertCompletionText(CurSelectedCandidate);
                return FReply::Handled();
            }
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
        
		if(FChar::IsIdentifier(Character) || Character == TEXT('.'))
        {
            bTryComplete = true;
        }
		
		bTryMergeUndoState = false;
		if(FChar::IsIdentifier(Character) || FChar::IsWhitespace(Character))
		{
			bTryMergeUndoState = true;
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
        TArray< FTextLayout::FLineModel >& Lines = ShaderTextLayout->GetLineModels();
        
		IsFoldEditTransaction = true;
		ShaderMultiLineEditableTextLayout->BeginEditTransation();
		
        TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
        
        const FTextLocation& CurCursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
        if (!MarkerIndex)
        {
            auto BraceGroup = ShaderMarshaller->FoldingBraceGroups[LineIndex];
            int32 FoldedBeginningRow = BraceGroup.OpenLineIndex;
            int32 FoldedBeginningCol = BraceGroup.OpenBrace.Offset;
            int32 FoldedEndRow = BraceGroup.CloseLineIndex;
            int32 FoldedEndCol = BraceGroup.CloseBrace.Offset;

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

            FString TextAfterMarker = BeginLine.Text->Mid(MarkerCol + 1);
			ShaderTextLayout->RemoveAt(FTextLocation{ LineIndex, MarkerCol }, BeginLine.Text->Len() - MarkerCol);
			ShaderMultiLineEditableText->GoTo(FTextLocation{ LineIndex, MarkerCol });
			FString FoldedText;
			for (int32 i = 0; i < FoldedLineText.Num(); i++)
			{
				FoldedText += FoldedLineText[i];
				if(i == FoldedLineText.Num() - 1)
				{
					FoldedText += TextAfterMarker;
				}
				else
				{
					FoldedText += "\n";
				}
			}
			ShaderMultiLineEditableText->InsertTextAtCursor(MoveTemp(FoldedText));

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
			else
			{
				ShaderMultiLineEditableText->GoTo(CurCursorLocation);
			}
        }
		
		ShaderMultiLineEditableTextLayout->EndEditTransaction();
		IsFoldEditTransaction = false;
		
        return FReply::Handled();
    }

	bool SShaderEditorBox::IsErrorLine(int InLineNumber) const
	{
		const int32 LineIndex = GetLineIndex(InLineNumber);
		std::optional<int> DiagLineNumber;
		if(EffectMarshller->LineNumberToDiagInfo.Contains(InLineNumber))
		{
			DiagLineNumber = InLineNumber;
		}
		else if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
		{
			int32 FoldedLineCounts = VisibleFoldMarkers[*MarkerIndex].GetFoldedLineCounts();
			for (int32 i = InLineNumber; i <= InLineNumber + FoldedLineCounts + 1; i++)
			{
				if (EffectMarshller->LineNumberToDiagInfo.Contains(i))
				{
					DiagLineNumber = i;
				}
			}
		}
		return DiagLineNumber ? EffectMarshller->LineNumberToDiagInfo[DiagLineNumber.value()].IsError : false;
		
	}

	bool SShaderEditorBox::IsWarningLine(int InLineNumber) const
	{
		const int32 LineIndex = GetLineIndex(InLineNumber);
		std::optional<int> DiagLineNumber;
		if(EffectMarshller->LineNumberToDiagInfo.Contains(InLineNumber))
		{
			DiagLineNumber = InLineNumber;
		}
		else if (TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex))
		{
			int32 FoldedLineCounts = VisibleFoldMarkers[*MarkerIndex].GetFoldedLineCounts();
			for (int32 i = InLineNumber; i <= InLineNumber + FoldedLineCounts + 1; i++)
			{
				if (EffectMarshller->LineNumberToDiagInfo.Contains(i))
				{
					DiagLineNumber = i;
				}
			}
		}
		return DiagLineNumber ? EffectMarshller->LineNumberToDiagInfo[DiagLineNumber.value()].IsWarn : false;
	}

    TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
        int32 LineNumber = FCString::Atoi(*(*Item).ToString());
        const int32 LineIndex = GetLineIndex(LineNumber);
		
		auto& CodeFontInfo = GetCodeFontInfo();
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
		
		auto LineMarker = SNew(SImage)
		.DesiredSizeOverride(FVector2D{12.0, 12.0})
		.Image_Lambda([=, this] {
			if(IsWarningLine(LineNumber) || IsErrorLine(LineNumber))
			{
				return FAppStyle::Get().GetBrush("Icons.Warning");
			}
			else if(StopLineNumber == LineNumber)
			{
				return FShaderHelperStyle::Get().GetBrush("Icons.ArrowBoldRight");
			}
			return FAppStyle::Get().GetBrush("Icons.BulletPoint");
		})
		.ColorAndOpacity_Lambda([=, this]() -> FSlateColor {
			if(IsWarningLine(LineNumber))
			{
				return FStyleColors::Warning;
			}
			else if(IsErrorLine(LineNumber))
			{
				return FStyleColors::Error;
			}
			else if(StopLineNumber == LineNumber && DebuggerError.IsEmpty())
			{
				return FLinearColor::Green;
			}
			return FLinearColor::Red;
		})
		.Visibility_Lambda([=, this] {
			if(IsErrorLine(LineNumber) || IsWarningLine(LineNumber))
			{
				return EVisibility::Visible;
			}
			else if(BreakPointLines.Contains(LineNumber) || StopLineNumber == LineNumber)
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
						SNew(SBox)
						.MinDesiredWidth(16.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							LineMarker
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
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
						if(BreakPointLines.Contains(LineNumber) || StopLineNumber == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber]{
							if(StopLineNumber == LineNumber && DebuggerError.IsEmpty())
							{
								return FLinearColor{0,1,0,0.7f};
							}
							return FLinearColor{1,0,0,0.7f};
						})
					]
				]
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber) || StopLineNumber == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.Image(FAppStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda([this, LineNumber]{
						if(StopLineNumber == LineNumber && DebuggerError.IsEmpty())
						{
							return FLinearColor{0,1,0,0.06f};
						}
						return FLinearColor{1,0,0,0.06f};
					})
				]
			];
        
		return LineNumberRow;
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
        //DummyTextBlock is used to keep the same layout as LineNumber and MultiLineEditableText.
        TSharedPtr<STextBlock> DummyTextBlock = SNew(STextBlock)
            .Font(GetCodeFontInfo())
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
						if(BreakPointLines.Contains(LineNumber) || StopLineNumber == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber]{
							if(StopLineNumber == LineNumber && DebuggerError.IsEmpty())
							{
								return FLinearColor{0,1,0,0.7f};
							}
							return FLinearColor{1,0,0,0.7f};
						})
					]
				]
				+SOverlay::Slot()
				[
					SNew(SBorder)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLines.Contains(LineNumber) || StopLineNumber == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.BorderImage_Lambda([this, LineNumber]{
						if(StopLineNumber == LineNumber && !DebuggerError.IsEmpty())
						{
							return FAppStyle::Get().GetBrush("WhiteBrush");
						}
						return FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect");
					})
					.BorderBackgroundColor_Lambda([this, LineNumber]{
						if(StopLineNumber == LineNumber && DebuggerError.IsEmpty())
						{
							return FLinearColor{0,1,0,0.3f};
						}
						return FLinearColor{1,0,0,0.3f};
					})
					.Padding(0)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(GetCodeFontInfo())
						.Visibility_Lambda([this, LineNumber] { return StopLineNumber == LineNumber && !DebuggerError.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
						.Text_Lambda([this] { return FText::FromString(DebuggerError); })
						.ColorAndOpacity(FLinearColor::Red)
					]
				]
            ];

    
        LineTip->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, LineNumber]{
            const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
            const int32 CurLineIndex = CursorLocation.GetLineIndex();
            auto FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
            if(FocusedWidget == ShaderMultiLineEditableText && LineNumber == GetLineNumber(CurLineIndex)
			   && !BreakPointLines.Contains(LineNumber) && StopLineNumber != LineNumber)
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

    FShaderEditorMarshaller::FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<HlslTokenizer> InTokenizer)
        : OwnerWidget(InOwnerWidget)
        , TextLayout(nullptr)
        , Tokenizer(MoveTemp(InTokenizer))
    {
       
    }

    void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels)
    {
        TextLayout = &TargetTextLayout;
		
		FString EditorSourceBeforeEditing = OwnerWidget->CurrentEditorSource;
		TArray<FTextRange> LineRangesBeforeEditing;
		FTextRange::CalculateLineRangesFromString(EditorSourceBeforeEditing, LineRangesBeforeEditing);
		
		OwnerWidget->CurrentEditorSource = SourceString;
		auto& LineModels = TextLayout->GetLineModels();
		FSlateEditableTextLayout* EditableTextLayout = OwnerWidget->ShaderMultiLineEditableTextLayout;
		if(OldLineModels.Num() == 0)
		{
			TArray<HlslTokenizer::TokenizedLine> TokenizedLines = Tokenizer->Tokenize(SourceString);
			
			TArray<FTextRange> LineRanges;
			FTextRange::CalculateLineRangesFromString(SourceString, LineRanges);

			TArray<FTextLayout::FNewLineData> LinesToAdd;
			for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); LineIndex++)
			{
				TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(LineRanges[LineIndex].BeginIndex, LineRanges[LineIndex].Len()));
				
				TArray<TSharedRef<IRun>> Runs;
				TOptional<int32> MarkerIndex = OwnerWidget->FindFoldMarker(LineIndex);
				for (const HlslTokenizer::Token& Token : TokenizedLines[LineIndex].Tokens)
				{
					FTextBlockStyle& RunTextStyle = OwnerWidget->GetTokenStyle(Token.Type);
					FTextRange NewTokenRange{ Token.BeginOffset, Token.EndOffset };

					if (MarkerIndex && LineText->Mid(NewTokenRange.BeginIndex, 1) == FoldMarkerText)
					{
						RunTextStyle.SetColorAndOpacity(FLinearColor::Gray);
					}

					Runs.Add(FSlateTextStyleRefRun::Create(FRunInfo(), LineText, RunTextStyle, MoveTemp(NewTokenRange)));
				}

				LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
			}

			TextLayout->AddLines(MoveTemp(LinesToAdd));
			
			for(int i = 0; i < LineModels.Num(); i++)
			{
				LineModels[i].CustomData = MakeShared<HlslTokenizer::TokenizedLine>(MoveTemp(TokenizedLines[i]));
			}
			
		}
		else
		{
			//Incremental processing
			//OldLineModels already were handled by internal frameworks here
			LineModels = MoveTemp(OldLineModels);
			bool bForce{};
			for(int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
			{
				if(bForce || (LineModels[LineIndex].DirtyFlags & FTextLayout::ELineModelDirtyState::All))
				{
					TSharedRef<FString> LineText = LineModels[LineIndex].Text;
					FString LineTextBeforeEditing;
					if(LineRangesBeforeEditing.IsValidIndex(LineIndex))
					{
						LineTextBeforeEditing = EditorSourceBeforeEditing.Mid(LineRangesBeforeEditing[LineIndex].BeginIndex, LineRangesBeforeEditing[LineIndex].Len());
					}
					HlslTokenizer::LineContState LastLineContState = HlslTokenizer::LineContState::None;
					HlslTokenizer::TokenizedLine* CurTokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
					if(LineIndex > 0)
					{
						HlslTokenizer::TokenizedLine* LastTokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex - 1].CustomData.Get());
						LastLineContState = LastTokenizedLine->State;
					}
					HlslTokenizer::TokenizedLine NewTokenizedLine = Tokenizer->Tokenize(*LineText, false, LastLineContState)[0];
					bForce = CurTokenizedLine ? NewTokenizedLine.State != CurTokenizedLine->State : true;
					
					TArray<FTextLayout::FRunModel> RunModels;
					TOptional<int32> MarkerIndex = OwnerWidget->FindFoldMarker(LineIndex);
					for (const HlslTokenizer::Token& Token : NewTokenizedLine.Tokens)
					{
						FTextBlockStyle* RunTextStyle = &OwnerWidget->GetTokenStyle(Token.Type);
						FTextRange NewTokenRange{ Token.BeginOffset, Token.EndOffset };
						
						if(!CurTokenizedLine)
						{
							CurTokenizedLine = &NewTokenizedLine;
						}
						//Compares old tokens to avoid lexical highlighting overriding syntax highlighting
						if(auto* MatchedToken = CurTokenizedLine->Tokens.FindByPredicate([&](const HlslTokenizer::Token& Element){
							FString TokenStr = LineText->Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
							FString ElementStr = LineTextBeforeEditing.Mid(Element.BeginOffset, Element.EndOffset - Element.BeginOffset);
							return Token.Type == HLSL::TokenType::Identifier && Token.Type == Element.Type
							&& (Token.BeginOffset == Element.BeginOffset || TokenStr == ElementStr);
						}))
						{
							if(OwnerWidget->LineSyntaxHighlightMapsCopy.IsValidIndex(LineIndex))
							{
								for(const auto& [Range, Style] : OwnerWidget->LineSyntaxHighlightMapsCopy[LineIndex])
								{
									if(Range.BeginIndex == MatchedToken->BeginOffset)
									{
										RunTextStyle = Style;
									}
								}
							}
						}

						if (MarkerIndex && LineText->Mid(NewTokenRange.BeginIndex, 1) == FoldMarkerText)
						{
							RunTextStyle->SetColorAndOpacity(FLinearColor::Gray);
						}

						TSharedRef<IRun> Run = FSlateTextStyleRefRun::Create(FRunInfo(), LineText, *RunTextStyle, MoveTemp(NewTokenRange));
						RunModels.Add(MoveTemp(Run));
					}
					
					LineModels[LineIndex].Runs = RunModels;
					LineModels[LineIndex].CustomData = MakeShared<HlslTokenizer::TokenizedLine>(MoveTemp(NewTokenizedLine));
				}
			}
		
		}
		
		SyntaxTask Task;
		Task.ShaderDesc = OwnerWidget->GetShaderAsset()->GetShaderDesc(SourceString);
		Task.LineTokens.SetNum(LineModels.Num());
		for(int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
		{
			HlslTokenizer::TokenizedLine* CurTokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
			for (const HlslTokenizer::Token& Token : CurTokenizedLine->Tokens)
			{
				Task.LineTokens[LineIndex].Add(Token);
			}
		}
		OwnerWidget->bRefreshSyntax.store(false, std::memory_order_relaxed);
		OwnerWidget->SyntaxQueue.Enqueue(MoveTemp(Task));
		OwnerWidget->SyntaxEvent->Trigger();

        TArray<HlslTokenizer::BraceGroup> BraceGroups;
		struct BraceStackData
		{
			int32 LineIndex{};
			HlslTokenizer::Brace Brace;
		};
        TArray<BraceStackData> OpenBraceStack;
        for(int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
        {
			HlslTokenizer::TokenizedLine* TokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
            for(const HlslTokenizer::Brace& Brace : TokenizedLine->Braces)
            {
                if(Brace.Type == HlslTokenizer::BraceType::Open)
                {
                    OpenBraceStack.Emplace(LineIndex, Brace);
                }
				else
				{
					if(!OpenBraceStack.IsEmpty())
					{
						auto OpenBraceData = OpenBraceStack.Pop();
						BraceGroups.Emplace(OpenBraceData.LineIndex, OpenBraceData.Brace, LineIndex, Brace);
					}
				}
            }
        }

        FoldingBraceGroups.Empty();
        for (const auto& BraceGroup : BraceGroups)
        {
            if (BraceGroup.OpenLineIndex != BraceGroup.CloseLineIndex && !FoldingBraceGroups.Contains(BraceGroup.OpenLineIndex)) {
                FoldingBraceGroups.Add(BraceGroup.OpenLineIndex, BraceGroup);
            }
        }
		
        if(EditableTextLayout && EditableTextLayout->CurrentUndoLevel >= 0)
        {
            ShaderUndoState* UndoState = static_cast<ShaderUndoState*>(EditableTextLayout->UndoStates[EditableTextLayout->CurrentUndoLevel].Get());
            OwnerWidget->VisibleFoldMarkers = UndoState->FoldMarkers;
			OwnerWidget->BreakPointLines = UndoState->BreakPointLines;
        }
        else
        {
            TArray<FoldMarker> NewFoldMarkers;
            for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
            {
				TSharedRef<FString> LineText = LineModels[LineIndex].Text;
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

        //Update fold markers highlight.
		TextLayout->ClearLineHighlights();
        for (const FTextLineHighlight& Highlight : Highlights)
        {
            TextLayout->AddLineHighlight(Highlight);
        }

        FTextSelection UnFoldingRange = FTextSelection{ {0, 0}, {LineModels.Num() - 1, LineModels[LineModels.Num() - 1].Text->Len()} };
        FString TextAfterUnfolding = OwnerWidget->UnFoldText(MoveTemp(UnFoldingRange));
		OwnerWidget->OnShaderTextChanged(TextAfterUnfolding);
        OwnerWidget->UpdateLineNumberData();
    }

    void FShaderEditorMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
    {
        SourceTextLayout.GetAsText(TargetString);
    }

    void FShaderEditorEffectMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels)
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
            ErrorInfoStyle.SetFont(OwnerWidget->GetCodeFontInfo());
			FTextBlockStyle WarnInfoStyle = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorWarnInfoText");
			WarnInfoStyle.SetFont(OwnerWidget->GetCodeFontInfo());
			
            FTextBlockStyle DummyInfoStyle = FTextBlockStyle{}
                .SetFont(OwnerWidget->GetCodeFontInfo())
                .SetColorAndOpacity(FLinearColor{ 0, 0, 0, 0 });

            int32 CurLineNumber = OwnerWidget->GetLineNumber(LineIndex);
            if (LineNumberToDiagInfo.Contains(CurLineNumber))
            {
                TSharedRef<FString> TotalInfo = MakeShared<FString>(LineNumberToDiagInfo[CurLineNumber].TotalInfo);

                TSharedRef<IRun> DummyRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, DummyInfoStyle, LineNumberToDiagInfo[CurLineNumber].DummyRange);
                Runs.Add(MoveTemp(DummyRun));

				if(LineNumberToDiagInfo[CurLineNumber].IsError)
				{
					TSharedRef<IRun> ErrorRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, ErrorInfoStyle, LineNumberToDiagInfo[CurLineNumber].InfoRange);
					Runs.Add(MoveTemp(ErrorRun));
				}
				else if(LineNumberToDiagInfo[CurLineNumber].IsWarn)
				{
					TSharedRef<IRun> WarnRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, WarnInfoStyle, LineNumberToDiagInfo[CurLineNumber].InfoRange);
					Runs.Add(MoveTemp(WarnRun));
				}

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

	void SShaderEditorBox::ApplyDebugState(const FW::SpvDebugState& State, bool bReverse)
	{
        CurReturnObject = State.ReturnObject;
		if(State.bFuncCall)
		{
			if(Scope)
			{
				CallStack.Add(TPair<SpvLexicalScope*, int>(Scope, State.Line));
				CurValidLine.reset();
			}
		}
		
		if(State.ScopeChange)
		{
			Scope = bReverse ? State.ScopeChange.value().PreScope : State.ScopeChange.value().NewScope;
			//Return func
			if(!CallStack.IsEmpty() && State.bReturn)
			{
				auto Item = CallStack.Pop();
				CurValidLine = Item.Value;
			}
		}
		
		for(const SpvVariableChange& VarChange : State.VarChanges)
		{
			auto& RecordedInfo = DebuggerContext->ThreadState.RecordedInfo;
			SpvVariable& Var = RecordedInfo.AllVariables[VarChange.VarId];
			if(!Var.IsExternal())
			{
				Var.InitializedRanges.Add({VarChange.Range.OffsetBytes, VarChange.Range.OffsetBytes + VarChange.Range.ByteSize});
				std::get<SpvObject::Internal>(Var.Storage).Value = bReverse ? VarChange.PreValue : VarChange.NewValue;
			}
			DirtyVars.Add(VarChange.VarId, VarChange.Range);
		}
	}

	TArray<VariableNodePtr> AppendVarChildNodes(SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<SpvVariableChange::DirtyRange>& DirtyRanges, const TArray<uint8>& Value, int32 InOffset)
	{
		TArray<VariableNodePtr> Nodes;
		int32 Offset = InOffset;
		if(TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(TypeDesc);
			int32 MemberByteSize = GetTypeByteSize(MemberTypeDesc);
			FString ValueStr = GetValueStr(Value, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset);
			auto Data = MakeShared<VariableNode>(MemberTypeDesc->GetName(), MoveTemp(ValueStr), GetTypeDescStr(MemberTypeDesc->GetTypeDesc()));
			if(MemberTypeDesc->GetTypeDesc()->GetKind() == SpvTypeDescKind::Composite)
			{
				Data->Children = AppendVarChildNodes(MemberTypeDesc->GetTypeDesc(), InitializedRanges, DirtyRanges, Value, Offset);
			}
			for(const auto& Range : DirtyRanges)
			{
				int32 RangeStart = Range.OffsetBytes;
				int32 RangeEnd = Range.OffsetBytes + Range.ByteSize;
				if((RangeStart >= InOffset && RangeStart < InOffset + MemberByteSize) ||
				   (RangeEnd > InOffset && RangeEnd <= InOffset + MemberByteSize) ||
				   (RangeStart < InOffset && RangeEnd > InOffset + MemberByteSize))
				{
					Data->Dirty = true;
					break;
				}
			}
			Nodes.Add(MoveTemp(Data));
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			for(SpvTypeDesc* MemberTypeDesc : CompositeTypeDesc->GetMemberTypeDescs())
			{
				Nodes.Append(AppendVarChildNodes(MemberTypeDesc, InitializedRanges, DirtyRanges, Value, Offset));
				if(MemberTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					Offset += GetTypeByteSize(MemberTypeDesc);
				}
			}
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(TypeDesc);
			SpvTypeDesc* BaseTypeDesc = ArrayTypeDesc->GetBaseTypeDesc();
			int32 BaseTypeSize = GetTypeByteSize(BaseTypeDesc);
			int32 CompCount = ArrayTypeDesc->GetCompCounts()[0];
			SpvTypeDesc* ElementTypeDesc = BaseTypeDesc;
			int32 ElementTypeSize = BaseTypeSize;
			TUniquePtr<SpvArrayTypeDesc> SubArrayTypeDesc = ArrayTypeDesc->GetCompCounts().Num() > 1 ? MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, TArray{ArrayTypeDesc->GetCompCounts().GetData() + 1, ArrayTypeDesc->GetCompCounts().Num() -1 }) : nullptr;
			if(SubArrayTypeDesc)
			{
				ElementTypeDesc = SubArrayTypeDesc.Get();
				ElementTypeSize *= SubArrayTypeDesc->GetElementNum();
			}
			for(int32 Index = 0; Index < CompCount; Index++)
			{
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				auto Data = MakeShared<VariableNode>(MemberName, MoveTemp(ValueStr), GetTypeDescStr(ElementTypeDesc));
				Data->Children = AppendVarChildNodes(ElementTypeDesc, InitializedRanges, DirtyRanges, Value, Offset);
				for(const auto& Range : DirtyRanges)
				{
					int32 RangeStart = Range.OffsetBytes;
					int32 RangeEnd = Range.OffsetBytes + Range.ByteSize;
					if((RangeStart >= Offset && RangeStart < Offset + ElementTypeSize) ||
					   (RangeEnd > Offset && RangeEnd <= Offset + ElementTypeSize) ||
					   (RangeStart < Offset && RangeEnd > Offset + ElementTypeSize))
					{
						Data->Dirty = true;
						break;
					}
				}
				Nodes.Add(MoveTemp(Data));
				Offset += ElementTypeSize;
			}
			
		}
		return Nodes;
	}

	TArray<ExpressionNodePtr> AppendExprChildNodes(SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, const TArray<uint8>& Value, int32 InOffset)
	{
		TArray<ExpressionNodePtr> Nodes;
		int32 Offset = InOffset;
		if(TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(TypeDesc);
			int32 MemberByteSize = GetTypeByteSize(MemberTypeDesc);
			FString ValueStr = GetValueStr(Value, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset);
			FString TypeName = GetTypeDescStr(MemberTypeDesc->GetTypeDesc());
			auto Data = MakeShared<ExpressionNode>(MemberTypeDesc->GetName(), FText::FromString(ValueStr), FText::FromString(TypeName));
			if(MemberTypeDesc->GetTypeDesc()->GetKind() == SpvTypeDescKind::Composite)
			{
				Data->Children = AppendExprChildNodes(MemberTypeDesc->GetTypeDesc(), InitializedRanges, Value, Offset);
			}
			Nodes.Add(MoveTemp(Data));
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			for(SpvTypeDesc* MemberTypeDesc : CompositeTypeDesc->GetMemberTypeDescs())
			{
				Nodes.Append(AppendExprChildNodes(MemberTypeDesc, InitializedRanges, Value, Offset));
				if(MemberTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					Offset += GetTypeByteSize(MemberTypeDesc);
				}
			}
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(TypeDesc);
			SpvTypeDesc* BaseTypeDesc = ArrayTypeDesc->GetBaseTypeDesc();
			int32 BaseTypeSize = GetTypeByteSize(BaseTypeDesc);
			int32 CompCount = ArrayTypeDesc->GetCompCounts()[0];
			SpvTypeDesc* ElementTypeDesc = BaseTypeDesc;
			int32 ElementTypeSize = BaseTypeSize;
			TUniquePtr<SpvArrayTypeDesc> SubArrayTypeDesc = ArrayTypeDesc->GetCompCounts().Num() > 1 ? MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, TArray{ArrayTypeDesc->GetCompCounts().GetData() + 1, ArrayTypeDesc->GetCompCounts().Num() -1 }) : nullptr;
			if(SubArrayTypeDesc)
			{
				ElementTypeDesc = SubArrayTypeDesc.Get();
				ElementTypeSize *= SubArrayTypeDesc->GetElementNum();
			}
			for(int32 Index = 0; Index < CompCount; Index++)
			{
				FString MemberName = FString::Printf(TEXT("[%d]"), Index);
				FString ValueStr = GetValueStr(Value, ElementTypeDesc, InitializedRanges, Offset);
				FString TypeName = GetTypeDescStr(ElementTypeDesc);
				auto Data = MakeShared<ExpressionNode>(MemberName, FText::FromString(ValueStr), FText::FromString(TypeName));
				Data->Children = AppendExprChildNodes(ElementTypeDesc, InitializedRanges, Value, Offset);
				Nodes.Add(MoveTemp(Data));
				Offset += ElementTypeSize;
			}
		}
		return Nodes;
	}

	void SShaderEditorBox::ShowDeuggerVariable(SpvLexicalScope* InScope) const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerVariableView* DebuggerLocalVariableView = ShEditor->GetDebuggerLocalVariableView();
        SDebuggerVariableView* DebuggerGlobalVariableView = ShEditor->GetDebuggerGlobalVariableView();
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
		
		TArray<VariableNodePtr> LocalVarNodeDatas;
        TArray<VariableNodePtr> GlobalVarNodeDatas;
		const auto& RecordedInfo = DebuggerContext->ThreadState.RecordedInfo;
		const auto& DebugStates = RecordedInfo.DebugStates;
		
		if(CurReturnObject && Scope == InScope)
		{
			SpvTypeDesc* ReturnTypeDesc = std::get<SpvTypeDesc*>(GetFunctionDesc(Scope)->GetFuncTypeDesc()->GetReturnType());
			const TArray<uint8>& Value = std::get<SpvObject::Internal>(CurReturnObject.value().Storage).Value;
			FString VarName = GetFunctionDesc(Scope)->GetName() + LOCALIZATION("ReturnTip").ToString();
			FString TypeName = GetTypeDescStr(ReturnTypeDesc);
			FString ValueStr = GetValueStr(Value, ReturnTypeDesc, TArray{Vector2i{0, Value.Num()}}, 0);
			
			auto Data = MakeShared<VariableNode>(VarName, ValueStr, TypeName);
			LocalVarNodeDatas.Add(MoveTemp(Data));
		}
		
		std::vector<std::pair<SpvId, SpvVariableDesc*>> SortedVariableDescs;
		for(const auto& Pair : DebuggerContext->VariableDescMap)
		{
			if(Pair.second)
			{
				SortedVariableDescs.push_back(Pair);
			}
		}
		std::sort(SortedVariableDescs.begin(), SortedVariableDescs.end(), [](const auto& PairA, const auto& PairB){
			return PairA.second->Line > PairB.second->Line;
		});
		
		for(const auto& [VarId, VarDesc] : SortedVariableDescs)
		{
			if(RecordedInfo.AllVariables.contains(VarId))
			{
				const SpvVariable& Var = RecordedInfo.AllVariables.at(VarId);
				if(!Var.IsExternal())
				{
					bool VisibleScope = VarDesc->Parent->Contains(InScope) || (DebugStates[CurDebugStateIndex].bReturn && InScope->GetKind() == SpvScopeKind::Function && InScope == VarDesc->Parent->GetParent());
					bool VisibleLine = VarDesc->Line < StopLineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
					if(VisibleScope && VisibleLine)
					{
						FString VarName = VarDesc->Name;
						FString TypeName = GetTypeDescStr(VarDesc->TypeDesc);
						const TArray<uint8>& Value = std::get<SpvObject::Internal>(Var.Storage).Value;
						FString ValueStr = GetValueStr(Value, VarDesc->TypeDesc, Var.InitializedRanges, 0);
						auto Data = MakeShared<VariableNode>(VarName, ValueStr, TypeName);
						TArray<SpvVariableChange::DirtyRange> Ranges;
						DirtyVars.MultiFind(VarId, Ranges);
						if(VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Composite || VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array)
						{
							Data->Children = AppendVarChildNodes(VarDesc->TypeDesc, Var.InitializedRanges, Ranges, Value, 0);
						}
						
						if(!Ranges.IsEmpty())
						{
							Data->Dirty = true;
						}

                        if(!VarDesc->bGlobal)
                        {
		                    LocalVarNodeDatas.Add(MoveTemp(Data));
                        }
                        else
                        {
                            GlobalVarNodeDatas.Add(MoveTemp(Data));
                        }
					}
				}
			}
			
		}
		
		DebuggerLocalVariableView->SetVariableNodeDatas(LocalVarNodeDatas);
        DebuggerGlobalVariableView->SetVariableNodeDatas(GlobalVarNodeDatas);
	}

	ExpressionNode SShaderEditorBox::EvaluateExpression(const FString& InExpression) const
	{
		const auto& RecordedInfo = DebuggerContext->ThreadState.RecordedInfo;
		const auto& DebugStates = RecordedInfo.DebugStates;
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
	
		FString ExpressionShader = ShaderAssetObj->GetShaderDesc(CurrentShaderSource).Source;
        TArray<uint8> InputData;
		
		//Patch EntryPoint
		FString EntryPoint = ShaderAssetObj->Shader->GetEntryPoint();
		for(const ShaderFuncScope& FuncScope : FuncScopesCopy)
		{
			if(FuncScope.Name == EntryPoint)
			{
				TArray<FTextRange> LineRanges;
				FTextRange::CalculateLineRangesFromString(ExpressionShader, LineRanges);
				int32 StartPos = LineRanges[FuncScope.Start.x - 1].BeginIndex + FuncScope.Start.y - 1;
				int32 EndPos = LineRanges[FuncScope.End.x - 1].BeginIndex + FuncScope.End.y - 1;
				
				ExpressionShader.RemoveAt(StartPos, EndPos - StartPos);
				
				//Append bindings
				FString LocalVars;
				FString VisibleLocalVarBindings = "struct __Expression_Vars_Set {";
				for(const auto& [VarId, VarDesc] : DebuggerContext->VariableDescMap)
				{
					if(RecordedInfo.AllVariables.contains(VarId))
					{
						const SpvVariable& Var = RecordedInfo.AllVariables.at(VarId);
						if(!Var.IsExternal() && VarDesc)
						{
							bool VisibleScope = VarDesc->Parent->Contains(Scope) || (DebugStates[CurDebugStateIndex].bReturn && Scope->GetKind() == SpvScopeKind::Function && Scope == VarDesc->Parent->GetParent());
							bool VisibleLine = VarDesc->Line < StopLineNumber + ExtraLineNum && VarDesc->Line > ExtraLineNum;
							if(VisibleScope && VisibleLine)
							{
								FString Declaration;
								if(VarDesc->TypeDesc->GetKind() == SpvTypeDescKind::Array)
								{
									SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(VarDesc->TypeDesc);
									FString BasicTypeStr = GetTypeDescStr(ArrayTypeDesc->GetBaseTypeDesc());
									FString DimStr = GetTypeDescStr(VarDesc->TypeDesc);
									DimStr.RemoveFromStart(BasicTypeStr);
									Declaration = BasicTypeStr + " " + VarDesc->Name + DimStr + ";";
									LocalVars += Declaration;
									LocalVars += VarDesc->Name + "= __Expression_Vars[0]." + VarDesc->Name + ";\n";
								}
								else
								{
									Declaration = GetTypeDescStr(VarDesc->TypeDesc) + " " + VarDesc->Name + ";";
									LocalVars += GetTypeDescStr(VarDesc->TypeDesc) + " " + VarDesc->Name + "= __Expression_Vars[0]." + VarDesc->Name + ";\n";
								}
								VisibleLocalVarBindings += Declaration;

                                const TArray<uint8>& Value = std::get<SpvObject::Internal>(Var.Storage).Value;
                                InputData.Append(Value);
							}
						}
					}
				}
				VisibleLocalVarBindings += "};\n";
				VisibleLocalVarBindings += "StructuredBuffer<__Expression_Vars_Set> __Expression_Vars : register(t114514);\n";
				
				//Append the EntryPoint
				FString EntryPointFunc = FString::Printf(TEXT("\nvoid %s() {\n"), *EntryPoint);
				EntryPointFunc += MoveTemp(LocalVars);
				EntryPointFunc += FString::Printf(TEXT("__Expression_Output(%s);\n"), *InExpression);
				EntryPointFunc += "}\n";
				ExpressionShader += VisibleLocalVarBindings + EntryPointFunc;
				break;
			}
		}
		
		static const FString OutputFunc =
R"(template<typename T>
void __Expression_Output(T __Expression_Result) {}
)";
		ExpressionShader = OutputFunc + ExpressionShader;
		
		auto ShaderDesc = GpuShaderSourceDesc{
			.Name = "EvaluateExpression",
			.Source = MoveTemp(ExpressionShader),
			.Type = ShaderAssetObj->Shader->GetShaderType(),
			.EntryPoint = EntryPoint
		};
		TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(ShaderDesc);
		Shader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		
		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs);
		if(ErrorInfo.IsEmpty())
		{
            TRefCountPtr<GpuBuffer> LocalVarsInput = GGpuRhi->CreateBuffer({
                .ByteSize = (uint32)InputData.Num(),
                .Usage = GpuBufferUsage::Storage,
                .Stride = (uint32)InputData.Num(),
                .InitialData = InputData
            });

            SpirvParser Parser;
		    Parser.Parse(Shader->SpvCode);
            if(Shader->GetShaderType() == ShaderType::PixelShader)
            {
                std::array<SpvVmContext,4> Quad;
                for(int i = 0; i < Quad.size(); i++)
                {       
                    Quad[i].ThreadState.BuiltInInput = VmPixelContext.value().Quad[i].ThreadState.BuiltInInput;
                    Quad[i].ThreadState.LocationInput = VmPixelContext.value().Quad[i].ThreadState.LocationInput;
					
					SpvMetaVisitor MetaVisitor{Quad[i]};
					Parser.Accept(&MetaVisitor);
                }

                TArray<SpvVmBinding> Bindings = VmPixelContext.value().Bindings;
                Bindings.Add(SpvVmBinding{
                    .DescriptorSet = 0,
                    .Binding = 114514,
                    .Resource = AUX::StaticCastRefCountPtr<GpuResource>(LocalVarsInput)
                });

                SpvVmPixelExprContext VmContext{
                    SpvVmPixelContext{
                        .DebugIndex = VmPixelContext.value().DebugIndex,
                        .Bindings = MoveTemp(Bindings),
                        .Quad = MoveTemp(Quad)
                    }
                };
                SpvVmPixelExprVisitor VmVisitor{VmContext};
		        Parser.Accept(&VmVisitor);
	
				if(!VmContext.HasSideEffect)
				{
					FString TypeName = GetTypeDescStr(VmContext.ResultTypeDesc);
					TArray<Vector2i> ResultRange;
					ResultRange.Add({0, VmContext.ResultValue.Num()});
					FString ValueStr = GetValueStr(VmContext.ResultValue, VmContext.ResultTypeDesc, ResultRange, 0);
					
					TArray<TSharedPtr<ExpressionNode>> Children;
					if(VmContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Composite || VmContext.ResultTypeDesc->GetKind() == SpvTypeDescKind::Array)
					{
						Children = AppendExprChildNodes(VmContext.ResultTypeDesc, ResultRange, VmContext.ResultValue, 0);
					}
					
					return {.Expr = InExpression, .ValueStr = FText::FromString(ValueStr),
						.TypeName = FText::FromString(TypeName), .Children = MoveTemp(Children)};
				}
				else
				{
					return {.Expr = InExpression, .ValueStr = LOCALIZATION("SideEffectExpr")};
				}
            }
		}

		return {.Expr = InExpression, .ValueStr = LOCALIZATION("InvalidExpr")};
	}

	void SShaderEditorBox::ShowDebuggerResult() const
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
		TArray<CallStackDataPtr> CallStackDatas;
		
		SpvFunctionDesc* FuncDesc = GetFunctionDesc(Scope);
		FString Location = FString::Printf(TEXT("%s (Line %d)"), *ShaderAssetObj->GetFileName(), CurValidLine.value() - ExtraLineNum);
		CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc), MoveTemp(Location)));
		for(int i = CallStack.Num() - 1; i >= 0; i--)
		{
			SpvFunctionDesc* FuncDesc = GetFunctionDesc(CallStack[i].Key);
			int JumpLineNumber = CallStack[i].Value - ExtraLineNum;
			if(JumpLineNumber > 0)
			{
				FString Location = FString::Printf(TEXT("%s (Line %d)"), *ShaderAssetObj->GetFileName(), JumpLineNumber);
				CallStackDatas.Add(MakeShared<CallStackData>(GetFunctionSig(FuncDesc), MoveTemp(Location)));
			}
		}
		DebuggerCallStackView->SetCallStackDatas(CallStackDatas);
		
		ShowDeuggerVariable(Scope);
		
		SDebuggerWatchView* DebuggerWatchView = ShEditor->GetDebuggerWatchView();
		DebuggerWatchView->OnWatch = [this](const FString& Expression) { return EvaluateExpression(Expression); };
		DebuggerWatchView->Refresh();
	}

	void SShaderEditorBox::Continue(StepMode Mode)
	{
		DirtyVars.Empty();
		DebuggerError.Empty();
		
		const auto& DebugStates = DebuggerContext->ThreadState.RecordedInfo.DebugStates;
		auto CallStackAtStop = CallStack;
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
		while(CurDebugStateIndex < DebugStates.Num())
		{
			const SpvDebugState& DebugState = DebugStates[CurDebugStateIndex];
			std::optional<int32> NextValidLine;
			if(CurDebugStateIndex + 1 < DebugStates.Num() &&
			   (!DebugStates[CurDebugStateIndex + 1].VarChanges.IsEmpty() || DebugStates[CurDebugStateIndex + 1].bReturn ||
				DebugStates[CurDebugStateIndex + 1].bFuncCallAfterReturn ||
				DebugStates[CurDebugStateIndex + 1].bFuncCall || DebugStates[CurDebugStateIndex + 1].bCondition))
			{
				NextValidLine = DebugStates[CurDebugStateIndex + 1].Line;
            
				//Stop at the ending line of the function lexcical scope
                if(NextValidLine.value() == 0 && DebugStates[CurDebugStateIndex + 1].bReturn)
                {
					for(const ShaderFuncScope& FuncScope : FuncScopesCopy)
					{
						if(FuncScope.Name == GetFunctionDesc(Scope)->GetName())
						{
							NextValidLine = FuncScope.End.x;
						}
					}
                }
			}
			
			if(!DebugState.UbError.IsEmpty())
			{
				DebuggerError = "Undefined behavior";
				StopLineNumber = DebugState.Line - ExtraLineNum;
				SH_LOG(LogShader, Error, TEXT("%s"), *DebugState.UbError);
				break;
			}
			
			ApplyDebugState(DebugState);
			CurDebugStateIndex++;
			
			if(AssertResult)
			{
				TArray<uint8>& Value = std::get<SpvObject::Internal>(AssertResult->Storage).Value;
				uint& TypedValue = *(uint*)Value.GetData();
				if(TypedValue != 1)
				{
					DebuggerError = "Assert failed";
					StopLineNumber = DebugState.Line - ExtraLineNum;
					TypedValue = 1;
					break;
				}
			}
			
			if(NextValidLine)
			{
				bool MatchBreakPoint = Scope && BreakPointLines.ContainsByPredicate([&](int32 InEntry){
					 int32 BreakPointRealLine = InEntry + ExtraLineNum;
					 bool bForward;
					 if(!CurValidLine)
					 {
						 int32 FuncLine = GetFunctionDesc(Scope)->GetLine();
						 bForward = (BreakPointRealLine >= FuncLine) && (BreakPointRealLine <= NextValidLine.value());
					 }
					 else
					 {
						 bForward = (BreakPointRealLine > CurValidLine.value()) && (BreakPointRealLine <= NextValidLine.value());
					 }
					 return bForward;
				});
				bool SameStack = CurValidLine != NextValidLine && CallStackAtStop.Num() == CallStack.Num();
				bool ReturnStack = CallStackAtStop.Num() > CallStack.Num();
				bool CrossStack = CallStackAtStop.Num() != CallStack.Num();
				
				bool StopStepOver = Mode == StepMode::StepOver && NextValidLine.value() - ExtraLineNum > 0 && (SameStack || ReturnStack);
				bool StopStepInto = Mode == StepMode::StepInto && NextValidLine.value() - ExtraLineNum > 0 && (SameStack || CrossStack);
				
				CurValidLine = NextValidLine;
				if(MatchBreakPoint || StopStepOver || StopStepInto)
				{
					CallStackAtStop = CallStack;
					StopLineNumber = NextValidLine.value() - ExtraLineNum;
					break;
				}
			}
		}
		
		if(CurDebugStateIndex >= DebugStates.Num())
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->EndDebugging();
		}
		else
		{
			ShowDebuggerResult();
		}
	}

	void SShaderEditorBox::ClearDebugger()
	{
		CurValidLine.reset();
		CallStack.Empty();
        DebuggerContext = nullptr;
		Scope = nullptr;
        CurReturnObject.reset();
		AssertResult = nullptr;
		DebuggerError.Empty();
		DirtyVars.Empty();
		StopLineNumber = 0;
        VmPixelContext.reset();
	}

	void SShaderEditorBox::DebugPixel(const FW::Vector2u& PixelCoord, const TArray<TRefCountPtr<FW::GpuBindGroup>>& BindGroups)
	{
		int32 DebugIndex= 2 * (PixelCoord.y & 1) + (PixelCoord.x & 1);
		
		TArray<SpvVmBinding> Bindings;
		for(const auto& BindGroup : BindGroups)
		{
			const auto& BindGroupDesc = BindGroup->GetDesc();
			int32 SetNumber = BindGroupDesc.Layout->GetGroupNumber();
			for(const auto& [Slot, ResourceBindingEntry] : BindGroupDesc.Resources)
			{
				const auto& LayoutBindingEntry = BindGroupDesc.Layout->GetDesc().Layouts[Slot];
				Bindings.Add({
					.DescriptorSet = SetNumber,
					.Binding = Slot,
					.Resource = ResourceBindingEntry.Resource
				});
			}
		}
		
		std::array<SpvVmContext,4> Quad;
		//TODO z,w
		uint32 QuadLeftTopX = PixelCoord.x & ~1u;
		uint32 QuadLeftTopY = PixelCoord.y & ~1u;
		Vector4f QuadFragCoord0 = {QuadLeftTopX + 0.5f, QuadLeftTopY + 0.5f, 0.0f, 1.0f};
		Vector4f QuadFragCoord1 = {QuadLeftTopX + 1.5f, QuadLeftTopY + 0.5f, 0.0f, 1.0f};
		Vector4f QuadFragCoord2 = {QuadLeftTopX + 0.5f, QuadLeftTopY + 1.5f, 0.0f, 1.0f};
		Vector4f QuadFragCoord3 = {QuadLeftTopX + 1.5f, QuadLeftTopY + 1.5f, 0.0f, 1.0f};
		Quad[0].ThreadState.BuiltInInput.emplace(SpvBuiltIn::FragCoord, TArray{(uint8*)&QuadFragCoord0, sizeof(Vector4f)});
		Quad[1].ThreadState.BuiltInInput.emplace(SpvBuiltIn::FragCoord, TArray{(uint8*)&QuadFragCoord1, sizeof(Vector4f)});
		Quad[2].ThreadState.BuiltInInput.emplace(SpvBuiltIn::FragCoord, TArray{(uint8*)&QuadFragCoord2, sizeof(Vector4f)});
		Quad[3].ThreadState.BuiltInInput.emplace(SpvBuiltIn::FragCoord, TArray{(uint8*)&QuadFragCoord3, sizeof(Vector4f)});
		
		TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(ShaderAssetObj->GetShaderDesc(CurrentShaderSource));
		Shader->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add("ENABLE_PRINT=0");
		
		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo, ExtraArgs);
		check(ErrorInfo.IsEmpty());
		
		SpirvParser Parser;
		Parser.Parse(Shader->SpvCode);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvMetaVisitor MetaVisitor{Quad[QuadIndex]};
			Parser.Accept(&MetaVisitor);
		}
		
		VmPixelContext = SpvVmPixelContext{
			DebugIndex,
			MoveTemp(Bindings),
			MoveTemp(Quad)
		};
		SpvVmPixelVisitor VmVisitor{VmPixelContext.value()};
		Parser.Accept(&VmVisitor);
		
		CurDebugStateIndex = 0;
		DebuggerContext = &VmPixelContext.value().Quad[DebugIndex];
		
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
	
		SDebuggerCallStackView* DebuggerCallStackView = ShEditor->GetDebuggerCallStackView();
		DebuggerCallStackView->OnSelectionChanged = [this](const FString& FuncName) {
			int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
			if(GetFunctionSig(GetFunctionDesc(Scope)) == FuncName)
			{
				StopLineNumber =  CurValidLine.value() - ExtraLineNum;
				ShowDeuggerVariable(Scope);
			}
			else if(auto Call = CallStack.FindByPredicate([this, FuncName](const TPair<FW::SpvLexicalScope*, int>& InItem){
				if(GetFunctionSig(GetFunctionDesc(InItem.Key)) == FuncName)
				{
					return true;
				}
				return false;
			}))
			{
				StopLineNumber = Call->Value - ExtraLineNum;
				ShowDeuggerVariable(Call->Key);
			}
		};
		for(const auto& [VarId, VarDesc] : DebuggerContext->VariableDescMap)
		{
			if(VarDesc && VarDesc->Name == "GPrivate_AssertResult")
			{
				AssertResult = &DebuggerContext->ThreadState.RecordedInfo.AllVariables.at(VarId);;
			}
		}
		Continue();
	
	}
}
