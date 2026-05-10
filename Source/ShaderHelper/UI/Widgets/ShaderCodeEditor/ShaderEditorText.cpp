#include "CommonHeader.h"
#include "UI/Widgets/ShaderCodeEditor/ShaderEditorText.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/ShaderCodeEditor/ShaderCodeEditorLineHighlighter.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/ColorPicker/SColorPicker.h"
#include "UI/Widgets/Debugger/SDebuggerVariableView.h"
#include "UI/Widgets/Misc/MiscWidget.h"

#include <Fonts/FontMeasure.h>
#include <Framework/Text/SlateTextRun.h>
#include <Styling/StyleColors.h>
#include <Widgets/Colors/SColorBlock.h>
#include <Widgets/Layout/SSeparator.h>
#include <Widgets/Text/SlateEditableTextLayout.h>

#include <regex>

using namespace FW;

namespace SH
{

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
    TArray<ShaderTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, true);
    const ShaderTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
    for (const auto& Token : TokenLine.Tokens)
    {
        if (InIndex > Token.BeginOffset && InIndex <= Token.EndOffset)
        {
            CurrentPosition = Token.BeginOffset;
            break;
        }
    }
    return CurrentPosition >= InIndex ? INDEX_NONE : CurrentPosition;
}

int32 TokenBreakIterator::MoveToCandidateAfter(const int32 InIndex)
{
    TArray<ShaderTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, true);
    const ShaderTokenizer::TokenizedLine& TokenLine = TokenizedLines[0];
    for (const auto& Token : TokenLine.Tokens)
    {
        if (InIndex >= Token.BeginOffset && InIndex < Token.EndOffset)
        {
            CurrentPosition = Token.EndOffset;
            break;
        }
    }
    return CurrentPosition <= InIndex ? INDEX_NONE : CurrentPosition;
}

int32 SShaderMultiLineEditableText::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const float InverseScale = Inverse(AllottedGeometry.Scale);

    if (Owner->CanShowGuideLine() && Owner->SyntaxTUCopy.IsValid())
    {
        TArray<ShaderScope> GuideLineScopes = Owner->SyntaxTUCopy->GetGuideLineScopes();
        int32 ExtraLineNum = Owner->GetShaderAsset()->GetExtraLineNum();
        //Draw guide lines
        for (const auto& Scope : GuideLineScopes)
        {
            int32 StartLineIndex = Scope.Start.X - ExtraLineNum - 1;
            int32 StartOffset = Scope.Start.Y - 1;
            int32 EndLineIndex = Scope.End.X - ExtraLineNum - 1;
            if (StartLineIndex >= 0 && StartLineIndex < EndLineIndex)
            {
                // Calculate position using uniform line height and measure actual text width
                float LineHeight = Owner->ShaderMarshaller->TextLayout->GetUniformLineHeight();
                float Scale = Owner->ShaderMarshaller->TextLayout->GetScale();
                FVector2D ScrollOffset = Owner->ShaderMultiLineEditableTextLayout->GetScrollOffset() * Scale;
                const FSlateFontInfo& FontInfo = Owner->GetCodeFontInfo();
                auto MeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

                // Measure actual text width up to StartOffset
                FString LineText;
                GetTextLine(StartLineIndex, LineText);
                FString Prefix = LineText.Left(StartOffset);
                constexpr float BaseFontSize = 10.0f;
                const double FontSizeScale = FontInfo.Size / BaseFontSize;
                const double ExtraOffset = 4.0 * Scale * FontSizeScale;
                double PosX = MeasureService->Measure(Prefix, FontInfo, Scale).X + ExtraOffset - ScrollOffset.X;
                double PosY = LineHeight * StartLineIndex - ScrollOffset.Y;

                FVector2D P0 = TransformPoint(InverseScale, FVector2D{ PosX, PosY });
                FVector2D P1 = TransformPoint(InverseScale, FVector2D{ PosX, PosY + LineHeight * (EndLineIndex - StartLineIndex) });
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), { P0, P1 },
                    ESlateDrawEffect::None, FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity(0.4f));
            }
        }

        // Draw a background layer for every non-whitespace block (only for visible region)
        const auto& LineViews = Owner->ShaderMarshaller->TextLayout->GetLineViews();
        int32 StartVisibleModelIndex = Owner->ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
        int32 EndVisibleModelIndex = Owner->ShaderMarshaller->TextLayout->GetEndVisibleLineIndex();
        for (int32 ViewIndex = 0; ViewIndex < LineViews.Num(); ++ViewIndex)
        {
            const FTextLayout::FLineView& LineView = LineViews[ViewIndex];
            // Skip lines outside visible model index range
            if (LineView.ModelIndex < StartVisibleModelIndex - 1 || LineView.ModelIndex > EndVisibleModelIndex)
            {
                continue;
            }
            for (const TSharedRef<ILayoutBlock>& Block : LineView.Blocks)
            {
                const FTextRange BlockRange = Block->GetTextRange();
                FString Text;
                GetTextLine(LineView.ModelIndex, Text);

                bool bIsOnlyWhitespace = true;
                for (int32 Index = BlockRange.BeginIndex; Index < BlockRange.EndIndex; ++Index)
                {
                    if (!FChar::IsWhitespace((*Text)[Index]))
                    {
                        bIsOnlyWhitespace = false;
                        break;
                    }
                }
                if (!bIsOnlyWhitespace && Owner->BackgroundLayerBrush)
                {
                    FSlateDrawElement::MakeBox(
                        OutDrawElements,
                        LayerId,
                        AllottedGeometry.ToPaintGeometry(TransformVector(InverseScale, Block->GetSize()), FSlateLayoutTransform(TransformPoint(InverseScale, Block->GetLocationOffset()))),
                        FCoreStyle::Get().GetBrush("WhiteBrush"),
                        ESlateDrawEffect::None,
                        Owner->BackgroundLayerBrush->TintColor.GetSpecifiedColor()
                    );
                }
            }
        }
    }

    LayerId = SMultiLineEditableText::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    return LayerId;
}

FShaderEditorMarshaller::FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<ShaderTokenizer> InTokenizer)
    : OwnerWidget(InOwnerWidget)
    , TextLayout(nullptr)
    , Tokenizer(MoveTemp(InTokenizer))
{
}

void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels)
{
    TextLayout = &TargetTextLayout;
    FString EditorSourceBeforeEditing = OwnerWidget->CurrentEditorSource;
    OwnerWidget->CurrentEditorSource = SourceString;
    auto& LineModels = TextLayout->GetLineModels();
    FSlateEditableTextLayout* EditableTextLayout = OwnerWidget->ShaderMultiLineEditableTextLayout;
    if (OldLineModels.Num() == 0)
    {
        TArray<ShaderTokenizer::TokenizedLine> TokenizedLines = Tokenizer->Tokenize(SourceString);

        TArray<FTextRange> LineRanges;
        FTextRange::CalculateLineRangesFromString(SourceString, LineRanges);

        TArray<FTextLayout::FNewLineData> LinesToAdd;
        for (int32 LineIndex = 0; LineIndex < TokenizedLines.Num(); LineIndex++)
        {
            TSharedRef<FString> LineText = MakeShared<FString>(SourceString.Mid(LineRanges[LineIndex].BeginIndex, LineRanges[LineIndex].Len()));

            TArray<TSharedRef<IRun>> Runs;
            TOptional<int32> MarkerIndex = OwnerWidget->FindFoldMarker(LineIndex);
            for (const ShaderTokenizer::Token& Token : TokenizedLines[LineIndex].Tokens)
            {
                FTextBlockStyle& RunTextStyle = OwnerWidget->GetTokenStyleMap()[Token.Type];
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

        for (int i = 0; i < LineModels.Num(); i++)
        {
            LineModels[i].CustomData = MakeShared<ShaderTokenizer::TokenizedLine>(MoveTemp(TokenizedLines[i]));
        }

    }
    //Incremental processing
    else
    {
        TArray<FTextRange> LineRangesBeforeEditing;
        FTextRange::CalculateLineRangesFromString(EditorSourceBeforeEditing, LineRangesBeforeEditing);

        //OldLineModels already were handled by internal frameworks here
        LineModels = MoveTemp(OldLineModels);
        bool bForce{};
        for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
        {
            if (bForce || (LineModels[LineIndex].DirtyFlags & FTextLayout::ELineModelDirtyState::All))
            {
                TSharedRef<FString> LineText = LineModels[LineIndex].Text;
                FString LineTextBeforeEditing;
                if (LineRangesBeforeEditing.IsValidIndex(LineIndex))
                {
                    LineTextBeforeEditing = EditorSourceBeforeEditing.Mid(LineRangesBeforeEditing[LineIndex].BeginIndex, LineRangesBeforeEditing[LineIndex].Len());
                }
                ShaderTokenizer::StateSet LastLineContState = ShaderTokenizer::StateSet::Start;
                ShaderTokenizer::TokenizedLine* CurTokenizedLine = static_cast<ShaderTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
                if (LineIndex > 0)
                {
                    ShaderTokenizer::TokenizedLine* LastTokenizedLine = static_cast<ShaderTokenizer::TokenizedLine*>(LineModels[LineIndex - 1].CustomData.Get());
                    LastLineContState = LastTokenizedLine->State;
                }
                ShaderTokenizer::TokenizedLine NewTokenizedLine = Tokenizer->Tokenize(*LineText, false, LastLineContState)[0];
                bForce = CurTokenizedLine ? NewTokenizedLine.State != CurTokenizedLine->State : true;

                TArray<FTextLayout::FRunModel> RunModels;
                TOptional<int32> MarkerIndex = OwnerWidget->FindFoldMarker(LineIndex);
                for (const ShaderTokenizer::Token& Token : NewTokenizedLine.Tokens)
                {
                    FTextRange NewTokenRange{ Token.BeginOffset, Token.EndOffset };
                    FString TokenStr = LineText->Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);

                    if (!CurTokenizedLine)
                    {
                        CurTokenizedLine = &NewTokenizedLine;
                    }

                    //Check if line is in inactive region
                    bool bIsInactive = false;
                    if (OwnerWidget->SyntaxTUCopy.IsValid())
                    {
                        TArray<Vector2u> InactiveLineRange = OwnerWidget->SyntaxTUCopy->GetInactiveRegions();
                        int32 ExtraLineNum = OwnerWidget->GetShaderAsset()->GetExtraLineNum();
                        int32 LineNumber = LineIndex + 1 + ExtraLineNum;
                        for (const Vector2u& Range : InactiveLineRange)
                        {
                            if (LineNumber >= (int32)Range.x && LineNumber <= (int32)Range.y)
                            {
                                bIsInactive = true;
                                break;
                            }
                        }
                    }
                    FTextBlockStyle* RunTextStyle = bIsInactive ? &OwnerWidget->GetDimTokenStyleMap()[Token.Type] : &OwnerWidget->GetTokenStyleMap()[Token.Type];
                    //Compares old tokens to avoid lexical highlighting overriding syntax highlighting
                    if (OwnerWidget->LineSyntaxHighlightMapsCopy.IsValidIndex(LineIndex))
                    {
                        for (const auto& [TokenRange, TokenType] : OwnerWidget->LineSyntaxHighlightMapsCopy[LineIndex])
                        {
                            FString OldTokenStr = LineTextBeforeEditing.Mid(TokenRange.BeginIndex, TokenRange.Len());
                            if (Token.Type == ShaderTokenType::Identifier && TokenStr == OldTokenStr)
                            {
                                RunTextStyle = bIsInactive ? &OwnerWidget->GetDimTokenStyleMap()[TokenType] : &OwnerWidget->GetTokenStyleMap()[TokenType];
                            }
                        }
                    }

                    TSharedRef<IRun> Run = FSlateTextStyleRefRun::Create(FRunInfo(), LineText, *RunTextStyle, MoveTemp(NewTokenRange));
                    RunModels.Add(MoveTemp(Run));
                }

                LineModels[LineIndex].Runs = RunModels;
                LineModels[LineIndex].CustomData = MakeShared<ShaderTokenizer::TokenizedLine>(MoveTemp(NewTokenizedLine));
            }
        }
    }

    this->TokenizedLines.Empty();
    for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
    {
        ShaderTokenizer::TokenizedLine* CurTokenizedLine = static_cast<ShaderTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
        this->TokenizedLines.Add(*CurTokenizedLine);
    }

    if (EditableTextLayout && EditableTextLayout->CurrentUndoLevel >= 0)
    {
        ShaderUndoState* UndoState = static_cast<ShaderUndoState*>(EditableTextLayout->UndoStates[EditableTextLayout->CurrentUndoLevel].Get());
        OwnerWidget->VisibleFoldMarkers = UndoState->FoldMarkers;
        OwnerWidget->BreakPointLineNumbers = UndoState->BreakPointLineNumbers;
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
                NewFoldMarkers.Add({ LineIndex, MarkerOffset });
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
        Highlights.Emplace(Marker.RelativeLineIndex, FTextRange{ Marker.Offset, Marker.Offset + 1 }, -1, FoldMarkerHighlighter::Create());
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

    //To prevent old diag info from obscuring the code.
    if (OwnerWidget->EffectMultiLineEditableText)
    {
        for (auto& [LineNumber, Info] : OwnerWidget->EffectMarshller->LineNumberToDiagInfo)
        {
            if (int32 LineIndex = OwnerWidget->GetLineIndex(LineNumber); LineIndex != INDEX_NONE)
            {
                int32 CodeLineLen = LineModels[LineIndex].Text->Len();
                FString DiagInfo = Info.TotalInfo.Mid(Info.InfoRange.BeginIndex);
                Info.DummyRange = { 0, CodeLineLen };
                FString NewDisplayInfo = *LineModels[LineIndex].Text + DiagInfo;
                Info.InfoRange = { CodeLineLen, NewDisplayInfo.Len() };
                Info.TotalInfo = MoveTemp(NewDisplayInfo);
            }
            else
            {
                OwnerWidget->EffectMarshller->LineNumberToDiagInfo.Remove(LineNumber);
            }
        }
        OwnerWidget->EffectMarshller->MakeDirty();
        OwnerWidget->EffectMultiLineEditableText->Refresh();
    }
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
    FTextBlockStyle ErrorInfoStyle = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorErrorInfoText");
    ErrorInfoStyle.SetFont(OwnerWidget->GetCodeFontInfo());
    FTextBlockStyle WarnInfoStyle = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorWarnInfoText");
    WarnInfoStyle.SetFont(OwnerWidget->GetCodeFontInfo());
    FTextBlockStyle DummyInfoStyle = FTextBlockStyle{}
        .SetFont(OwnerWidget->GetCodeFontInfo())
        .SetColorAndOpacity(FLinearColor{ 0, 0, 0, 0 });
    for (int32 LineIndex = 0; LineIndex < OwnerWidget->GetCurDisplayLineCount(); LineIndex++)
    {
        TArray<TSharedRef<IRun>> Runs;
        int32 CurLineNumber = OwnerWidget->GetLineNumber(LineIndex);
        if (LineNumberToDiagInfo.Contains(CurLineNumber))
        {
            TSharedRef<FString> TotalInfo = MakeShared<FString>(LineNumberToDiagInfo[CurLineNumber].TotalInfo);

            TSharedRef<IRun> DummyRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, DummyInfoStyle, LineNumberToDiagInfo[CurLineNumber].DummyRange);
            Runs.Add(MoveTemp(DummyRun));

            if (LineNumberToDiagInfo[CurLineNumber].IsError)
            {
                TSharedRef<IRun> ErrorRun = FSlateTextRun::Create(FRunInfo(), TotalInfo, ErrorInfoStyle, LineNumberToDiagInfo[CurLineNumber].InfoRange);
                Runs.Add(MoveTemp(ErrorRun));
            }
            else if (LineNumberToDiagInfo[CurLineNumber].IsWarn)
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

void SShaderMultiLineEditableText::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
    TSharedPtr<SWindow> ShaderEditorTipWindow = ShEditor->GetShaderEditorTipWindow();

    FVector2D ScreenSpaceCursorPos = FSlateApplication::Get().GetCursorPos();
    FGeometry WindowGeometry = ShaderEditorTipWindow->GetWindowGeometryInScreen();
    bool bMouseInTipWindow = ShaderEditorTipWindow->IsVisible() && WindowGeometry.IsUnderLocation(ScreenSpaceCursorPos);
    bool IsTopLevel = FSlateApplication::Get().GetActiveTopLevelRegularWindow() == FSlateApplication::Get().FindWidgetWindow(Owner->AsShared());
    bool bHasActiveMenu = FSlateApplication::Get().AnyMenusVisible();

    static double LastCheckTime = 0.0;
    const double CheckInterval = 0.4;

    bool bShouldCheck = InCurrentTime - LastCheckTime >= CheckInterval;
    bShouldCheck = bShouldCheck && !bMouseInTipWindow;

    //Show the debug value when hovering over variables
    auto CheckDebuggerTip = [&] {
        ExpressionNodePtr HoverExpr;
        FTextLocation CurrentHoverLocation;
        FString CurrentTokenName;
        int32 CurrentTokenBeginOffset = -1;
        int32 CurrentTokenEndOffset = -1;

        static ExpressionNodePtr LastHoverExpr;
        static int32 LastHoverLineIndex = -1;
        static FString LastTokenName;
        static int32 LastTokenBeginOffset = -1;
        static int32 LastTokenEndOffset = -1;

        if (AllottedGeometry.IsUnderLocation(ScreenSpaceCursorPos))
        {
            FVector2D LocalSpaceCursorPos = AllottedGeometry.AbsoluteToLocal(ScreenSpaceCursorPos);
            CurrentHoverLocation = Owner->ShaderMarshaller->TextLayout->GetTextLocationAt(LocalSpaceCursorPos * AllottedGeometry.Scale);
            auto& Debugger = static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebugger();
            if (Debugger.IsValid())
            {
                int32 ExtraLineNum = Owner->GetShaderAsset()->GetExtraLineNum();
                const auto& TokenizedLine = Owner->ShaderMarshaller->TokenizedLines[CurrentHoverLocation.GetLineIndex()];
                FString CurTextLine;
                GetTextLine(CurrentHoverLocation.GetLineIndex(), CurTextLine);

                //bool bFollowedBySquareBracket{};
                for (const auto& Token : TokenizedLine.Tokens)
                {
                    FString TokenName = CurTextLine.Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
                    if (CurrentHoverLocation.GetOffset() >= Token.BeginOffset && CurrentHoverLocation.GetOffset() <= Token.EndOffset)
                    {
                        if (Token.Type == ShaderTokenType::Identifier /*|| TokenName == "]"*/)
                        {
                            CurrentTokenName = MoveTemp(TokenName);
                            CurrentTokenBeginOffset = Token.BeginOffset;
                            CurrentTokenEndOffset = Token.EndOffset;
                            /*if (CurTextLine.Mid(Token.EndOffset, 1) == "]")
                            {
                                bFollowedBySquareBracket = true;
                            }*/
                            break;
                        }
                    }
                }

                if (!CurrentTokenName.IsEmpty())
                {
                    //Extract expression
                    int32 BeginOffset = CurrentTokenBeginOffset;
                    //while (BeginOffset > 0)
                    //{
                    //  TCHAR PrevChar = CurTextLine[BeginOffset - 1];
                    //  if (FChar::IsIdentifier(PrevChar) || PrevChar == TEXT('.') ||
                    //      PrevChar == TEXT('[') || PrevChar == TEXT(']'))
                    //  {
                    //      //If hovering between [ and ]
                    //      if (bFollowedBySquareBracket && PrevChar == TEXT('['))
                    //      {
                    //          break;
                    //      }
                    //      BeginOffset--;
                    //  }
                    //  else
                    //  {
                    //      break;
                    //  }
                    //}

                    FString Expression = CurTextLine.Mid(BeginOffset, CurrentTokenEndOffset - BeginOffset);
                    if (LastHoverExpr && LastHoverExpr->Expr == Expression)
                    {
                        HoverExpr = LastHoverExpr;
                    }
                    else
                    {
                        ExpressionNode EvalResult = Debugger.EvaluateExpression(Expression);
                        if (EvalResult.ValueStr != LOCALIZATION("InvalidExpr").ToString())
                        {
                            HoverExpr = MakeShared<ExpressionNode>(MoveTemp(EvalResult));
                        }
                    }
                }
            }
        }

        bool bSameToken = (CurrentHoverLocation.GetLineIndex() == LastHoverLineIndex &&
            CurrentTokenName == LastTokenName &&
            CurrentTokenBeginOffset == LastTokenBeginOffset &&
            CurrentTokenEndOffset == LastTokenEndOffset);

        if (HoverExpr)
        {
            if (!bSameToken)
            {
                auto VarView = SNew(SDebuggerVariableView)
                    .Font_Lambda([] { return SShaderEditorBox::GetCodeFontInfo(); })
                    .AutoWidth(true)
                    .HasHeaderRow(false);
                VarView->SetVariableNodeDatas({ HoverExpr });
                ShaderEditorTipWindow->SetContent(VarView);
                ShaderEditorTipWindow->Resize(FVector2D::ZeroVector);
                ShaderEditorTipWindow->ShowWindow();
                FVector2D BlockPos = Owner->ShaderMarshaller->TextLayout->GetLocationAt({ CurrentHoverLocation.GetLineIndex(), CurrentTokenBeginOffset }, true) / AllottedGeometry.Scale;
                FVector2D BlockScreenPos = AllottedGeometry.LocalToAbsolute(BlockPos);
                ShaderEditorTipWindow->MoveWindowTo(BlockScreenPos);

                LastHoverExpr = HoverExpr;
                LastHoverLineIndex = CurrentHoverLocation.GetLineIndex();
                LastTokenName = CurrentTokenName;
                LastTokenBeginOffset = CurrentTokenBeginOffset;
                LastTokenEndOffset = CurrentTokenEndOffset;
            }
        }
        else
        {
            LastHoverExpr.Reset();
            LastHoverLineIndex = -1;
            LastTokenName.Empty();
            LastTokenBeginOffset = -1;
            LastTokenEndOffset = -1;
        }
        return HoverExpr.IsValid();
    };

    auto CheckColorBlockTip = [&] {
        if (!SShaderEditorBox::CanShowColorBlock())
        {
            return false;
        }

        if (AllottedGeometry.IsUnderLocation(ScreenSpaceCursorPos))
        {
            FVector2D LocalSpaceCursorPos = AllottedGeometry.AbsoluteToLocal(ScreenSpaceCursorPos);
            FTextLocation CurrentHoverLocation = Owner->ShaderMarshaller->TextLayout->GetTextLocationAt(LocalSpaceCursorPos * AllottedGeometry.Scale);

            FString CurTextLine;
            GetTextLine(CurrentHoverLocation.GetLineIndex(), CurTextLine);
            std::string Str = TCHAR_TO_UTF8(*CurTextLine);
            std::regex Pattern(
                R"([\(\{}]\s*)"                                                      // ( or {
                R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"         // X
                R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"         // Y
                R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*)"             // z
                R"((?:\s*,\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?))?)"    // optional w
                R"(\s*[\)\}])"                                                       // ) or }
            );
            std::smatch Match;
            static FTextLocation LastMatchedPos;
            static std::string LastMatchedStr;
            if (std::regex_search(Str, Match, Pattern) && Match.size() >= 4)
            {
                if (CurrentHoverLocation.GetOffset() >= Match.position(0) &&
                    CurrentHoverLocation.GetOffset() < Match.position(0) + Match.length(0))
                {
                    FTextLocation CurMatchedPos = { CurrentHoverLocation.GetLineIndex(), (int32)Match.position(0) };
                    std::string CurMatchedStr = Match[0].str();
                    if (LastMatchedPos != CurMatchedPos || LastMatchedStr != CurMatchedStr)
                    {
                        LastMatchedPos = CurMatchedPos;
                        LastMatchedStr = CurMatchedStr;
                        float X = std::stof(Match[1].str());
                        float Y = std::stof(Match[2].str());
                        float Z = std::stof(Match[3].str());
                        float W = 1.0f;
                        bool HasAlpha = Match.size() > 4 && Match[4].matched && Match[4].length() > 0;
                        if (HasAlpha)
                        {
                            W = std::stof(Match[4].str());
                        }
                        X = FMath::Clamp(X, 0.0f, 1.0f);
                        Y = FMath::Clamp(Y, 0.0f, 1.0f);
                        Z = FMath::Clamp(Z, 0.0f, 1.0f);
                        W = FMath::Clamp(W, 0.0f, 1.0f);
                        FVector2D BlockBeginPos = Owner->ShaderMarshaller->TextLayout->GetLocationAt(CurMatchedPos, true) / AllottedGeometry.Scale;
                        FVector2D BlockEndPos = Owner->ShaderMarshaller->TextLayout->GetLocationAt({ CurrentHoverLocation.GetLineIndex(), (int32)Match.position(0) + (int32)Match.length(0) }, true) / AllottedGeometry.Scale;
                        FVector2D BlockBeginScreenPos = AllottedGeometry.LocalToAbsolute(BlockBeginPos);
                        auto ColorBlock = SNew(SBox)
                            .WidthOverride(BlockEndPos.X - BlockBeginPos.X)
                            .HeightOverride(SShaderEditorBox::GetFontSize() + 4)
                            [
                                SNew(SBorder)
                                [
                                    SNew(SColorBlock)
                                    .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
                                    .ShowBackgroundForAlpha(true)
                                    .Color(FLinearColor{ X, Y, Z, W })
                                    .UseSRGB(false) //TODO
                                    .OnMouseButtonDown_Lambda([=, this](const FGeometry&, const FPointerEvent& MouseEvent) {
                                        ShaderEditorTipWindow->SetContent(
                                            SNew(SColorPicker)
                                            .TargetColorAttribute(FLinearColor{ X, Y, Z, W })
                                            .ShowAlpha(HasAlpha)
                                            .OnColorChanged_Lambda([CurrentHoverLocation, Pattern, this](const FLinearColor& InColor) {
                                                //Modify LineModel directly instead of calling BeginTransaction/EndTransaction to avoid pushing an undo state.
                                                auto& Lines = Owner->ShaderMarshaller->TextLayout->GetLineModels();
                                                auto& ColorBlockLineModel = Lines[CurrentHoverLocation.GetLineIndex()];
                                                FString& LineText = *ColorBlockLineModel.Text;
                                                std::string Str = TCHAR_TO_UTF8(*LineText);
                                                std::smatch Match;
                                                if (std::regex_search(Str, Match, Pattern) && Match.size() >= 4)
                                                {
                                                    bool HasAlpha = Match.size() > 4 && Match[4].matched && Match[4].length() > 0;

                                                    FString OpenBrace = FString::Chr(Match[0].str()[0]);
                                                    FString CloseBrace = FString::Chr(Match[0].str().back());

                                                    FString NewColorStr;
                                                    if (HasAlpha)
                                                        NewColorStr = FString::Printf(TEXT("%s%.3f, %.3f, %.3f, %.3f%s"), *OpenBrace, InColor.R, InColor.G, InColor.B, InColor.A, *CloseBrace);
                                                    else
                                                        NewColorStr = FString::Printf(TEXT("%s%.3f, %.3f, %.3f%s"), *OpenBrace, InColor.R, InColor.G, InColor.B, *CloseBrace);

                                                    int32 ReplacePos = (int32)Match.position(0);
                                                    int32 ReplaceLen = (int32)Match.length(0);
                                                    LineText = LineText.Left(ReplacePos) + NewColorStr + LineText.Mid(ReplacePos + ReplaceLen);

                                                    ColorBlockLineModel.DirtyFlags |= FTextLayout::ELineModelDirtyState::All;
                                                    Owner->ShaderMarshaller->MakeDirty();
                                                    Owner->ShaderMultiLineEditableText->Refresh();
                                                }
                                            })
                                            .OnDestroyed_Lambda([CurTextLine, CurrentHoverLocation, this] {
                                                //Only manually push an an undo state here.
                                                auto& Lines = Owner->ShaderMarshaller->TextLayout->GetLineModels();
                                                auto& ColorBlockLineModel = Lines[CurrentHoverLocation.GetLineIndex()];
                                                FString& LineText = *ColorBlockLineModel.Text;
                                                if (CurTextLine != LineText)
                                                {
                                                    auto NewUndoState = Owner->ShaderMultiLineEditableTextLayout->MakeUndoState();
                                                    NewUndoState->Commands.Add(MakeShared<FRemoveAtCmd>(FTextLocation{ CurrentHoverLocation.GetLineIndex(), 0 }, CurTextLine));
                                                    NewUndoState->Commands.Add(MakeShared<FInsertAtCmd>(FTextLocation{ CurrentHoverLocation.GetLineIndex(), 0 }, LineText));
                                                    Owner->ShaderMultiLineEditableTextLayout->PushUndoState(MoveTemp(NewUndoState));
                                                }

                                            })
                                        );
                                        return FReply::Handled();
                                    })
                                ]
                            ];
                        ShaderEditorTipWindow->SetContent(ColorBlock);
                        ShaderEditorTipWindow->Resize(FVector2D::ZeroVector);
                        ShaderEditorTipWindow->ShowWindow();
                        ShaderEditorTipWindow->MoveWindowTo(BlockBeginScreenPos);
                    }
                    return true;
                }
                else
                {
                    LastMatchedPos = {};
                    LastMatchedStr.clear();
                }
            }
            else
            {
                LastMatchedPos = {};
                LastMatchedStr.clear();
            }
        }
        return false;
    };

    auto CheckQuickInfo = [&] {
        if (!SShaderEditorBox::CanShowQuickInfo())
        {
            return false;
        }

        if (AllottedGeometry.IsUnderLocation(ScreenSpaceCursorPos) && !static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebugger().IsValid())
        {
            FVector2D LocalSpaceCursorPos = AllottedGeometry.AbsoluteToLocal(ScreenSpaceCursorPos);
            FTextLocation CurrentHoverLocation = Owner->ShaderMarshaller->TextLayout->GetTextLocationAt(LocalSpaceCursorPos * AllottedGeometry.Scale);
            int32 ExtraLineNum = Owner->GetShaderAsset()->GetExtraLineNum();
            const auto& TokenizedLine = Owner->ShaderMarshaller->TokenizedLines[CurrentHoverLocation.GetLineIndex()];
            int32 TokenOffset{};
            int32 TokenEndOffset{};

            static int32 LastHoverLineIndex = -1;
            static int32 LastTokenOffset = -1;

            for (const auto& Token : TokenizedLine.Tokens)
            {
                if ((Token.Type == ShaderTokenType::Identifier || Token.Type == ShaderTokenType::Preprocess)
                    && CurrentHoverLocation.GetOffset() >= Token.BeginOffset && CurrentHoverLocation.GetOffset() <= Token.EndOffset)
                {
                    TokenOffset = Token.BeginOffset;
                    TokenEndOffset = Token.EndOffset;
                    break;
                }
            }

            bool bSameToken = (CurrentHoverLocation.GetLineIndex() == LastHoverLineIndex &&
                TokenOffset == LastTokenOffset);

            if (Owner->SyntaxTUCopy.IsValid())
            {
                auto Symbol = Owner->SyntaxTUCopy->GetSymbolInfo(CurrentHoverLocation.GetLineIndex() + ExtraLineNum + 1, TokenOffset + 1, TokenEndOffset - TokenOffset);
                if (Symbol.Tokens.Num() > 0)
                {
                    if (!bSameToken)
                    {
                        static int32 CurrentOverloadIndex;
                        static TArray<ShaderSymbol::FuncOverload> Overloads;
                        Overloads = Symbol.Overloads;
                        CurrentOverloadIndex = 0;

                        auto TokensBox = SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("(") + LOCALIZATION(magic_enum::enum_name(Symbol.Type).data()).ToString() + TEXT(") ")))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", SShaderEditorBox::GetFontSize()))
                            ];
                        for (const auto& Token : Symbol.Tokens)
                        {
                            TokensBox->AddSlot().AutoWidth()
                            [
                                SNew(STextBlock)
                                    .Text(FText::FromString(Token.Key))
                                    .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                    .TextStyle(&SShaderEditorBox::GetTokenStyleMap()[Token.Value])
                            ];
                        }

                        auto Box = SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                TokensBox
                            ];

                        if (Symbol.ExpandedTokens.Num() > 0)
                        {
                            Box->AddSlot().AutoHeight()[SNew(SSeparator).Thickness(1.0f).ColorAndOpacity(FStyleColors::Border)];
                            auto ExpandedBox = SNew(SHorizontalBox);
                            for (const auto& Token : Symbol.ExpandedTokens)
                            {
                                ExpandedBox->AddSlot().AutoWidth()
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString(Token.Key))
                                        .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                        .TextStyle(&SShaderEditorBox::GetTokenStyleMap()[Token.Value])
                                ];
                            }
                            Box->AddSlot().AutoHeight()[ExpandedBox];
                        }

                        if (Overloads.Num() > 0 || !Symbol.Desc.IsEmpty())
                        {
                            Box->AddSlot().AutoHeight()[SNew(SSeparator).Thickness(1.0f).ColorAndOpacity(FStyleColors::Border)];

                            // Overloads navigation
                            if (Overloads.Num() > 0)
                            {
                                auto OverloadTokensBox = SNew(SHorizontalBox);
                                auto ParamsBox = SNew(SVerticalBox);

                                auto BuildOverloadTokens = [](TSharedRef<SHorizontalBox> InBox, const TArray<TPair<FString, ShaderTokenType>>& Tokens) {
                                    InBox->ClearChildren();
                                    for (const auto& Token : Tokens)
                                    {
                                        InBox->AddSlot().AutoWidth()
                                            [
                                                SNew(STextBlock)
                                                    .Text(FText::FromString(Token.Key))
                                                    .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                                    .TextStyle(&SShaderEditorBox::GetTokenStyleMap()[Token.Value])
                                            ];
                                    }
                                    };

                                auto BuildParamsBox = [](TSharedRef<SVerticalBox> InBox, const TArray<ShaderParameter>& Params) {
                                    InBox->ClearChildren();
                                    for (const auto& Param : Params)
                                    {
                                        if (Param.Desc.IsEmpty())
                                        {
                                            continue;
                                        }

                                        auto ParamRow = SNew(SHorizontalBox);

                                        // Bullet point
                                        ParamRow->AddSlot().AutoWidth()
                                            [
                                                SNew(STextBlock).Text(FText::FromString(TEXT("\u2022 ")))
                                                    .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                            ];

                                        // SemaFlag (in/out/inout)
                                        if (Param.SemaFlag != ParamSemaFlag::None)
                                        {
                                            FString SemaStr;
                                            switch (Param.SemaFlag)
                                            {
                                            case ParamSemaFlag::In:    SemaStr = TEXT("in"); break;
                                            case ParamSemaFlag::Out:   SemaStr = TEXT("out"); break;
                                            case ParamSemaFlag::Inout: SemaStr = TEXT("inout"); break;
                                            default: break;
                                            }
                                            ParamRow->AddSlot().AutoWidth()
                                            [
                                                SNew(STextBlock)
                                                    .Text(FText::FromString(SemaStr + TEXT(" ")))
                                                    .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                                    .TextStyle(&SShaderEditorBox::GetTokenStyleMap()[ShaderTokenType::Keyword])
                                            ];
                                        }

                                        // Parameter name
                                        ParamRow->AddSlot().AutoWidth()
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(Param.Name))
                                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", SShaderEditorBox::GetFontSize()))
                                        ];

                                        // Description
                                        ParamRow->AddSlot().AutoWidth()
                                        [
                                            SNew(STextBlock)
                                                .Text(FText::FromString(TEXT(" \u2014 ") + Param.Desc))
                                                .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                        ];

                                        InBox->AddSlot().AutoHeight()[ParamRow];
                                    }
                                    };

                                BuildOverloadTokens(OverloadTokensBox, Overloads[CurrentOverloadIndex].Tokens);
                                BuildParamsBox(ParamsBox, Overloads[CurrentOverloadIndex].Params);

                                Box->AddSlot().AutoHeight()
                                [
                                    SNew(SHorizontalBox)
                                    + SHorizontalBox::Slot()
                                    .AutoWidth()
                                    .VAlign(VAlign_Center)
                                    [
                                        SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.ChevronUp")).IconSize(FVector2D{ (float)SShaderEditorBox::GetFontSize(), (float)SShaderEditorBox::GetFontSize() })
                                        .OnClicked_Lambda([OverloadTokensBox, ParamsBox, BuildOverloadTokens, BuildParamsBox]() {
                                            if (Overloads.Num() > 0)
                                            {
                                                CurrentOverloadIndex = (CurrentOverloadIndex - 1 + Overloads.Num()) % Overloads.Num();
                                                BuildOverloadTokens(OverloadTokensBox, Overloads[CurrentOverloadIndex].Tokens);
                                                BuildParamsBox(ParamsBox, Overloads[CurrentOverloadIndex].Params);
                                            }
                                            return FReply::Handled();
                                        })
                                    ]
                                    + SHorizontalBox::Slot()
                                    .AutoWidth()
                                    .VAlign(VAlign_Center)
                                    .Padding(2, 0, 2, 0)
                                    [
                                        SNew(STextBlock)
                                        .Text_Lambda([]() {
                                            return FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentOverloadIndex + 1, Overloads.Num()));
                                        })
                                        .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                    ]
                                    + SHorizontalBox::Slot()
                                    .AutoWidth()
                                    .VAlign(VAlign_Center)
                                    [
                                        SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.ChevronDown")).IconSize(FVector2D{ (float)SShaderEditorBox::GetFontSize(), (float)SShaderEditorBox::GetFontSize() })
                                        .OnClicked_Lambda([OverloadTokensBox, ParamsBox, BuildOverloadTokens, BuildParamsBox]() {
                                            if (Overloads.Num() > 0)
                                            {
                                                CurrentOverloadIndex = (CurrentOverloadIndex + 1) % Overloads.Num();
                                                BuildOverloadTokens(OverloadTokensBox, Overloads[CurrentOverloadIndex].Tokens);
                                                BuildParamsBox(ParamsBox, Overloads[CurrentOverloadIndex].Params);
                                            }
                                            return FReply::Handled();
                                        })
                                    ]
                                    + SHorizontalBox::Slot()
                                    .AutoWidth()
                                    .VAlign(VAlign_Center)
                                    .Padding(4, 0, 0, 0)
                                    [
                                        OverloadTokensBox
                                    ]
                                ];

                                Box->AddSlot().AutoHeight()[ParamsBox];
                            }

                            if (!Symbol.Desc.IsEmpty())
                            {
                                Box->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
                                    [
                                        SNew(STextBlock)
                                            .Text(FText::FromString(Symbol.Desc))
                                            .Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                                    ];
                            }
                        }

                        int32 SymbolExtraLineNum = ExtraLineNum;
                        AssetPtr<ShaderAsset> SymbolAsset = Owner->GetShaderAsset()->FindIncludeAsset(Symbol.File);
                        if (SymbolAsset)
                        {
                            SymbolExtraLineNum = SymbolAsset->GetExtraLineNum();
                        }
                        int SymbolLineNumber = Symbol.Row - SymbolExtraLineNum;
                        if (SymbolLineNumber > 0 || !Symbol.Url.IsEmpty())
                        {
                            Box->AddSlot().AutoHeight()[SNew(SSeparator).Thickness(1.0f).ColorAndOpacity(FStyleColors::Border)];

                            auto LocationBox = SNew(SHorizontalBox);
                            FString LocationText = "Defined at: ";

                            if (!Symbol.File.IsEmpty() && SymbolLineNumber > 0)
                            {
                                LocationText += Symbol.File.IsEmpty()
                                    ? FString::Printf(TEXT("Line %d"), Symbol.Row)
                                    : FString::Printf(TEXT("%s:%d"), *Symbol.File, SymbolLineNumber);
                            }

                            LocationBox->AddSlot().AutoWidth().VAlign(VAlign_Center)
                            [
                                SNew(STextBlock).Text(FText::FromString(LocationText)).Font(FCoreStyle::GetDefaultFontStyle("NormalText", SShaderEditorBox::GetFontSize()))
                            ];

                            if (!Symbol.Url.IsEmpty())
                            {
                                LocationBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
                                [
                                    SNew(SIconButton)
                                    .Icon(FShaderHelperStyle::Get().GetBrush("Icons.Networking"))
                                    .IconSize(FVector2D{ (float)SShaderEditorBox::GetFontSize(), (float)SShaderEditorBox::GetFontSize() })
                                    .OnClicked_Lambda([Url = Symbol.Url]() {
                                        FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
                                        return FReply::Handled();
                                    })
                                ];
                            }
                            Box->AddSlot().AutoHeight()[LocationBox];
                        }

                        auto DocView = SNew(SBorder)
                        [
                            Box
                        ];
                        ShaderEditorTipWindow->SetContent(DocView);
                        ShaderEditorTipWindow->Resize(FVector2D::ZeroVector);
                        ShaderEditorTipWindow->ShowWindow();
                        FVector2D BlockPos = Owner->ShaderMarshaller->TextLayout->GetLocationAt({ CurrentHoverLocation.GetLineIndex(), TokenOffset }, true) / AllottedGeometry.Scale;
                        FVector2D BlockScreenPos = AllottedGeometry.LocalToAbsolute(BlockPos);
                        ShaderEditorTipWindow->MoveWindowTo(BlockScreenPos);

                        LastHoverLineIndex = CurrentHoverLocation.GetLineIndex();
                        LastTokenOffset = TokenOffset;
                    }
                    return true;
                }
                else
                {
                    LastHoverLineIndex = 0;
                    LastTokenOffset = 0;
                }
            }
        }
        return false;
    };

    if (bShouldCheck)
    {
        if (bHasActiveMenu || !IsTopLevel || (!CheckDebuggerTip() && !CheckColorBlockTip() && !CheckQuickInfo()))
        {
            ShaderEditorTipWindow->SetContent(SNullWidget::NullWidget);
            ShaderEditorTipWindow->HideWindow();
        }
        LastCheckTime = InCurrentTime;
    }
}

FReply SShaderMultiLineEditableText::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (FSlateApplication::Get().GetModifierKeys().IsControlDown() && SShaderEditorBox::CanMouseWheelZoom())
    {
        int32 FontSize = (int32)(SShaderEditorBox::GetFontSize() + MouseEvent.GetWheelDelta());
        Editor::GetEditorConfig()->SetInt64(TEXT("CodeEditor"), TEXT("FontSize"), FMath::Max(SShaderEditorBox::MinFontSize, FontSize));
        Editor::SaveEditorConfig();
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        for (auto ShaderEditor : ShEditor->GetShaderEditors())
        {
            ShaderEditor->RefreshFont();
        }
        return FReply::Handled();
    }
    return SMultiLineEditableText::OnMouseWheel(MyGeometry, MouseEvent);
}

FReply SShaderMultiLineEditableText::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MouseEvent.IsControlDown())
    {
        FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        FTextLocation ClickLocation = Owner->ShaderMarshaller->TextLayout->GetTextLocationAt(LocalMousePos * MyGeometry.Scale);
        Owner->GoToDefinitionAt(ClickLocation);
        return FReply::Handled();
    }
    return SMultiLineEditableText::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SShaderMultiLineEditableText::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    for (int32 i = 0; i < Owner->VisibleFoldMarkers.Num(); i++)
    {
        const TArray<FTextLayout::FLineModel>& Lines = Owner->ShaderMarshaller->TextLayout->GetLineModels();
        int FoldMarkerLineIndex = Owner->VisibleFoldMarkers[i].RelativeLineIndex;
        const auto& FoldMarkerLine = Lines[FoldMarkerLineIndex];
        for (const auto& Highlight : FoldMarkerLine.LineHighlights)
        {
            if (auto Highlighter = dynamic_cast<FoldMarkerHighlighter*>(&*Highlight.Highlighter))
            {
                FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
                const double Ext = 5.5;
                if (LocalMousePos.X >= Highlighter->Location.X - Ext &&
                    LocalMousePos.X < Highlighter->Location.X + Highlighter->Size.X + Ext &&
                    LocalMousePos.Y >= Highlighter->Location.Y - Ext &&
                    LocalMousePos.Y < Highlighter->Location.Y + Highlighter->Size.Y + Ext)
                {
                    Owner->UnFold(Owner->GetLineNumber(FoldMarkerLineIndex));
                    return FReply::Handled();
                }
            }
        }
    }
    return SMultiLineEditableText::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
}

}
