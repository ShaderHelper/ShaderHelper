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
#include "Editor/CodeEditorCommands.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "Editor/ShaderHelperEditor.h"
#include <Widgets/Text/SlateTextBlockLayout.h>
#include <Framework/Text/SlateTextHighlightRunRenderer.h>
#include <Fonts/FontMeasure.h>
#include <Styling/StyleColors.h>
#include "Editor/AssetEditor/AssetEditor.h"
#include "Common/Path/BaseResourcePath.h"
#include <Widgets/Colors/SColorBlock.h>
#include "UI/Widgets/ColorPicker/SColorPicker.h"
#include <regex>

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>, CursorLineHighlighter)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, SlateEditableTextTypes::FCursorInfo, CursorInfo)
STEAL_PRIVATE_MEMBER(STextBlock, TUniquePtr< FSlateTextBlockLayout >, TextLayoutCache)
STEAL_PRIVATE_MEMBER(FSlateTextBlockLayout, TSharedPtr<FSlateTextLayout>, TextLayout)
STEAL_PRIVATE_MEMBER(FTextLayout, uint8, DirtyFlags)

using namespace FW;

namespace SH
{

const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
const FLinearColor HighlightLineNumberTextColor = { 0.7f,0.7f,0.7f,0.9f };

const FLinearColor NormalLineTipColor = { 1.0f,1.0f,1.0f,0.0f };
const FLinearColor HighlightLineTipColor = { 1.0f,1.0f,1.0f,0.2f };

const FString FoldMarkerText = TEXT("⇿");

//Append dummy datas to LineNumberData for Padding
constexpr int PaddingLineNum = 22;

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
        TArray<HlslTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, true);
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
        TArray<HlslTokenizer::TokenizedLine> TokenizedLines = Marshaller->Tokenizer->Tokenize(InternalString, true);
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
    
	SShaderEditorBox::SShaderEditorBox()
	: Debugger(this)
	{
	}

	SShaderEditorBox::~SShaderEditorBox()
    {
		ShaderAssetObj->OnRefreshBuilder.Remove(RefreshBuilderHandle);

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

	FString SShaderEditorBox::GetFontPath()
	{
		FString FontPath = BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansMono.ttf");
		Editor::GetEditorConfig()->GetString(TEXT("CodeEditor"), TEXT("Font"), FontPath);
		return FontPath;
	}

	int32 SShaderEditorBox::GetFontSize()
	{
		int32 FontSize = 10;
		Editor::GetEditorConfig()->GetInt(TEXT("CodeEditor"), TEXT("FontSize"), FontSize);
		return FontSize;
	}

	int32 SShaderEditorBox::GetTabSize()
	{
		int32 TabSize = 4;
		Editor::GetEditorConfig()->GetInt(TEXT("CodeEditor"), TEXT("TabSize"), TabSize);
		return TabSize;
	}

	bool SShaderEditorBox::CanMouseWheelZoom()
	{
		bool MouseWheelZoom = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("MouseWheelZoom"), MouseWheelZoom);
		return MouseWheelZoom;
	}

	bool SShaderEditorBox::CanShowColorBlock()
	{
		bool ShowColorBlock = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("ShowColorBlock"), ShowColorBlock);
		return ShowColorBlock;
	}

	FSlateFontInfo& SShaderEditorBox::GetCodeFontInfo()
	{
		static FSlateFontInfo CodeFontInfo;
		return CodeFontInfo;
	}

	void SShaderEditorBox::RefreshFont()
	{
		auto& CodeFontInfo = GetCodeFontInfo();
		TSharedRef<FCompositeFont> CodeFont = MakeShared<FStandaloneCompositeFont>();
		FString FontPath = GetFontPath();
		int32 FontSize = GetFontSize();
		Editor::GetEditorConfig()->GetString(TEXT("CodeEditor"), TEXT("Font"), FontPath);
		Editor::GetEditorConfig()->GetInt(TEXT("CodeEditor"), TEXT("FontSize"), FontSize);

		CodeFont->DefaultTypeface.AppendFont(TEXT("Code"), FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		CodeFont->FallbackTypeface.Typeface.AppendFont(TEXT("Code"), BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansFallback.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		CodeFontInfo = FSlateFontInfo(CodeFont, FontSize);

		for (auto& [_, Style] : GetTokenStyleMap())
		{
			Style.SetFont(CodeFontInfo);
		}
		ShaderMultiLineEditableText->SetFont(CodeFontInfo);
		//After marking Marshaller as dirty, calling refresh directly will rehandle the entire text, 
		//which may cause stuttering. Therefore, we only update layout
		ShaderMarshaller->ClearDirty();
		int32 StartVisibleLineIndex = ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
		double X = ShaderMultiLineEditableTextLayout->GetScrollOffset().X;
		double Y = ShaderMarshaller->TextLayout->GetUniformLineHeight() * StartVisibleLineIndex / ShaderMarshaller->TextLayout->GetScale();
		ShaderMultiLineEditableTextLayout->SetScrollOffset({ X,Y }, ShaderMultiLineEditableText->GetTickSpaceGeometry());
		ShaderMarshaller->TextLayout->ResetMaxDrawWidth();
		ShaderMarshaller->TextLayout->UpdateIfNeeded();
		ShaderMultiLineEditableTextLayout->Tick(ShaderMultiLineEditableText->GetTickSpaceGeometry(), 0, 0);

		EffectMultiLineEditableText->SetFont(CodeFontInfo);
		EffectMultiLineEditableText->Refresh();

		LineNumberList->RebuildList();
		LineTipList->RebuildList();

	}

	TMap<HLSL::TokenType, FTextBlockStyle>& SShaderEditorBox::GetTokenStyleMap()
	{
		static TMap<HLSL::TokenType, FTextBlockStyle> TokenStyleMap{
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
		return TokenStyleMap;
	}

	int32 SShaderMultiLineEditableText::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		const float InverseScale = Inverse(AllottedGeometry.Scale);
		const double OffsetX = 4;

		TArray<ShaderScope> Scopes = Owner->SyntaxTU.GetScopes();
		int32 ExtraLineNum = Owner->GetShaderAsset()->GetExtraLineNum();
		//Draw guide lines
		for (const auto& Scope : Scopes)
		{
			int32 StartLineIndex = Scope.Start.X - ExtraLineNum - 1;
			int32 StartOffset = Scope.Start.Y - 1;
			if (StartLineIndex >= 0)
			{
				int32 EndLineIndex = Scope.End.X - ExtraLineNum - 1;
				TSharedPtr<ILayoutBlock> Block = Owner->ShaderMarshaller->TextLayout->GetBlockAt({ StartLineIndex, StartOffset });
				if (Block)
				{
					FVector2D P0 = TransformPoint(InverseScale, Block->GetLocationOffset() + FVector2D{OffsetX, 2.0});
					FVector2D P1 = TransformPoint(InverseScale, Block->GetLocationOffset() + FVector2D{OffsetX, Block->GetSize().Y * (EndLineIndex - StartLineIndex)});
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), {P0, P1}, ESlateDrawEffect::None, FLinearColor{1,1,1,0.2f});
				}
			}
		}

		// Draw a background layer for every non-whitespace block
		const auto& LineViews = Owner->ShaderMarshaller->TextLayout->GetLineViews();
		for (int32 LineIndex = 0; LineIndex < LineViews.Num(); ++LineIndex)
		{
			const FTextLayout::FLineView& LineView = LineViews[LineIndex];
			for (const TSharedRef< ILayoutBlock >& Block : LineView.Blocks)
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

		LayerId = SMultiLineEditableText::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		return LayerId;
	}

	FVector2D SShaderEditorBox::GetCodeCompletionCanvasSize() const
	{
		const FSlateFontInfo Font = GetCodeFontInfo();
		const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const float LineH = (float)Measure->Measure(TEXT("M"), Font).Y;
		const float ItemH = LineH + 2.0f;
		const int32 Visible = FMath::Clamp(CandidateItems.Num(), 0, 10);
		const float Height = ItemH * Visible;
		const float CharW = (float)Measure->Measure(TEXT("M"), Font).X;
		const float Width = CharW * 24 + 20.0f;
		return FVector2D{ Width, Height };
	}

    void SShaderEditorBox::Construct(const FArguments& InArgs)
    {
		ShaderAssetObj = InArgs._ShaderAssetObj;
		RefreshBuilderHandle = ShaderAssetObj->OnRefreshBuilder.AddLambda([this]{
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

		//Due to code folding altering the editing text, separate IsenseThread and SyntaxThread.
		ISenseEvent = FPlatformProcess::GetSynchEventFromPool();
		ISenseThread = MakeUnique<FThread>(TEXT("ISenseThread"), [this] {
			while (!bQuitISense)
			{
				ISenseTask Task;
				while (ISenseQueue.Dequeue(Task));
				if (Task.ShaderDesc)
				{
					TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(Task.ShaderDesc.value());
					ShaderTU TU{ Shader->GetSourceText(), Shader->GetIncludeDirs() };
					DiagnosticInfos = TU.GetDiagnostic();

					CandidateInfos.Reset();
					if (!Task.CursorToken.IsEmpty())
					{
						CandidateInfos = TU.GetCodeComplete(Task.Row, Task.Col);
						if (Task.IsMemberAccess == false)
						{
							for (const auto& Candidate : DefaultCandidates())
							{
								CandidateInfos.AddUnique(Candidate);
							}
						}

						for (auto It = CandidateInfos.CreateIterator(); It; ++It)
						{
							if (Task.IsMemberAccess)
							{
								if ((*It).Text.Find("operator") != INDEX_NONE)
								{
									It.RemoveCurrent();
									continue;
								}
								else if ((*It).Kind == HLSL::CandidateKind::Type)
								{
									It.RemoveCurrent();
									continue;
								}
							}

							if (Task.CursorToken != ".")
							{
								if (!FuzzyMatch((*It).Text, *Task.CursorToken))
								{
									It.RemoveCurrent();
								}
							}
						}

						CandidateInfos.Sort([&](const ShaderCandidateInfo& A, const ShaderCandidateInfo& B) {
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
					this->ISenseTU = MoveTemp(TU);

					if (ISenseQueue.IsEmpty())
					{
						bRefreshIsense.store(true, std::memory_order_release);
					}
				}

				if (ISenseQueue.IsEmpty())
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
				while (SyntaxQueue.Dequeue(Task));
				if (Task.ShaderDesc)
				{
					TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(Task.ShaderDesc.value());
					ShaderTU TU{ Shader->GetSourceText(), Shader->GetIncludeDirs() };
					LineSyntaxHighlightMaps.Reset();
					LineSyntaxHighlightMaps.SetNum(Task.LineTokens.Num());
					int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
					for (int LineIndex = 0; LineIndex < Task.LineTokens.Num(); LineIndex++)
					{
						for (const HlslTokenizer::Token& Token : Task.LineTokens[LineIndex])
						{
							HLSL::TokenType NewTokenType = TU.GetTokenType(Token.Type, ExtraLineNum + LineIndex + 1, Token.BeginOffset + 1);
							if (NewTokenType != Token.Type)
							{
								LineSyntaxHighlightMaps[LineIndex].Add(FTextRange{ Token.BeginOffset, Token.EndOffset }, NewTokenType);
							}
						}
					}
					this->SyntaxTU = MoveTemp(TU);

					if (SyntaxQueue.IsEmpty())
					{
						bRefreshSyntax.store(true, std::memory_order_release);
					}
				}

				if (SyntaxQueue.IsEmpty())
				{
					SyntaxEvent->Wait();
				}
			}
		});
        
        auto CustomScrollBar = SNew(SScrollBar).Padding(0)
            .Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar"));
		BackgroundLayerBrush = FAppStyle::Get().GetBrush("Brushes.Recessed");
        ChildSlot
        [
            SNew(SBorder)
            .BorderImage(BackgroundLayerBrush)
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
							SAssignNew(ShaderMultiLineEditableText, SShaderMultiLineEditableText, this)
							.Text(InitialShaderText)
							.EnableUniformFont(true)
							.Font(GetCodeFontInfo())
							.Marshaller(ShaderMarshaller)
							.VScrollBar(ShaderMultiLineVScrollBar)
							.HScrollBar(ShaderMultiLineHScrollBar)
							.OnKeyCharHandler(this, &SShaderEditorBox::HandleKeyChar)
							.OnKeyDownHandler(this, &SShaderEditorBox::HandleKeyDown)
							.CreateSlateTextLayout_Lambda([this](SWidget* InOwner, FTextBlockStyle InDefaultTextStyle){
								auto TextLayout = MakeShared<ShaderTextLayout>(InOwner, InDefaultTextStyle, ShaderMarshaller.Get());
								TextLayout->KeepMaxDrawWidth(true);
								return TextLayout;
							})
							.OnIsTypedCharValid_Lambda([](const TCHAR InChar) { return true; })
							.OnCursorMoved(this, &SShaderEditorBox::OnCursorMoved)
							.OnContextMenuOpening_Lambda([this] {
								FMenuBuilder MenuBuilder(true, UICommandList);
								{
									MenuBuilder.BeginSection("EditText", FText::FromString("Edit Text"));
									{
										MenuBuilder.AddMenuEntry(CodeEditorCommands::Get().Cut, NAME_None, {}, {},
											FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Cut" });
										MenuBuilder.AddMenuEntry(CodeEditorCommands::Get().Copy, NAME_None, {}, {}, 
											FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Copy" });
										MenuBuilder.AddMenuEntry(CodeEditorCommands::Get().Paste, NAME_None, {}, {},
											FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "GenericCommands.Paste" });
									}
									MenuBuilder.EndSection();
								}
								return MenuBuilder.MakeWidget();
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
							.Size(this, &SShaderEditorBox::GetCodeCompletionCanvasSize)
							.Position_Lambda([this]{
								FVector2D TipPos = CustomCursorHighlighter->ScaledCursorPos;
								TipPos.Y += CustomCursorHighlighter->ScaledLineHeight;
								FVector2D Area = ShaderMultiLineEditableText->GetTickSpaceGeometry().GetLocalSize();
								FVector2D CanvasSize = GetCodeCompletionCanvasSize();
								if(TipPos.Y + CanvasSize.Y > Area.Y)
								{
									TipPos.Y -= CustomCursorHighlighter->ScaledLineHeight;
									TipPos.Y -= CanvasSize.Y;
								}
								if(TipPos.X + CanvasSize.X > Area.X)
								{
									TipPos.X -= CanvasSize.X;
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
			UndoState->BreakPointLineNumbers = BreakPointLineNumbers;
            return UndoState;
        };

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
        
		UICommandList = MakeShared<FUICommandList>();
        UICommandList->MapAction(
			CodeEditorCommands::Get().Save,
            FExecuteAction::CreateLambda([this] {
				ShaderAssetObj->Save();
                Compile();
            })
        );
		UICommandList->MapAction(CodeEditorCommands::Get().Undo,
			FExecuteAction::CreateLambda([this] { ShaderMultiLineEditableTextLayout->Undo(); }),
			FCanExecuteAction::CreateLambda([this] { return ShaderMultiLineEditableTextLayout->CanExecuteUndo(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().Redo,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->Redo();
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().Cut,
			FExecuteAction::CreateLambda([this] { CutText(); }),
			FCanExecuteAction::CreateLambda([this] { return !ShaderMultiLineEditableText->IsTextReadOnly(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().Paste,
			FExecuteAction::CreateLambda([this] { PasteText(); }),
			FCanExecuteAction::CreateLambda([this] { return ShaderMultiLineEditableTextLayout->CanExecutePaste(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().Copy,
			FExecuteAction::CreateLambda([this] { CopyText(); })
		);
		UICommandList->MapAction(CodeEditorCommands::Get().DeleteLeft,
			FExecuteAction::CreateLambda([this] { 
				SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
				if (!ShaderMultiLineEditableText->AnyTextSelected())
				{
					// if we are deleting a single open brace
					// look for a matching close brace following it and delete the close brace as well
					const FTextLocation& CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
					const int Offset = CursorLocation.GetOffset();
					FString Line;
					ShaderMultiLineEditableText->GetTextLine(CursorLocation.GetLineIndex(), Line);

					if (Line.IsValidIndex(Offset - 1))
					{
						if (IsOpenBrace(Line[Offset - 1]))
						{
							if (Line.IsValidIndex(Offset) && Line[Offset] == GetMatchedCloseBrace(Line[Offset - 1]))
							{
								ShaderMultiLineEditableText->SelectText(FTextLocation(CursorLocation, -1), FTextLocation(CursorLocation, 1));
								ShaderMultiLineEditableText->DeleteSelectedText();
								ShaderMultiLineEditableText->GoTo(FTextLocation(CursorLocation, -1));
								return;
							}
						}

						if (Line[Offset - 1] == FoldMarkerText[0])
						{
							UnFold(GetLineNumber(CursorLocation.GetLineIndex()));
							return;
						}

					}

				}
				else
				{
					const FTextSelection& Selection = ShaderMultiLineEditableText->GetSelection();
					int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
					int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
					for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
					{
						RemoveFoldMarker(LineIndex);
					}
				}
				
				ShaderMultiLineEditableTextLayout->HandleBackspace();
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().DeleteRight,
			FExecuteAction::CreateLambda([this]{
				SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
				ShaderMultiLineEditableTextLayout->HandleDelete();
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().DeleteTokenLeft,
			FExecuteAction::CreateLambda([this] {
				SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(-1, 0),
					ECursorAction::SelectText
				));
				ShaderMultiLineEditableText->DeleteSelectedText();
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().DeleteTokenRight,
			FExecuteAction::CreateLambda([this] {
				SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(+1, 0),
					ECursorAction::SelectText
				));
				ShaderMultiLineEditableText->DeleteSelectedText();
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(CodeEditorCommands::Get().SelectAll,
			FExecuteAction::CreateLambda([this] {ShaderMultiLineEditableTextLayout->SelectAllText(); }),
			FCanExecuteAction::CreateLambda([this] { return ShaderMultiLineEditableTextLayout->CanExecuteSelectAll(); })
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().ToggleComment,
			FExecuteAction::CreateRaw(this, &SShaderEditorBox::ToggleComment)
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().GoToBeginningOfLine,
			FExecuteAction::CreateLambda([this] { ShaderMultiLineEditableTextLayout->GoTo(ETextLocation::BeginningOfCodeLine); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().GoToEndOfLine,
			FExecuteAction::CreateLambda([this] { ShaderMultiLineEditableTextLayout->GoTo(ETextLocation::EndOfLine); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorSelectLeft,
			FExecuteAction::CreateLambda([this] { 
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Character,
					FIntPoint(-1, 0),
					ECursorAction::SelectText
				)); 
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorSelectRight,
			FExecuteAction::CreateLambda([this] { ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Character,
					FIntPoint(+1, 0),
					ECursorAction::SelectText
				)); 
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorSelectLineStart,
			FExecuteAction::CreateLambda([this] { 
				ShaderMultiLineEditableTextLayout->JumpTo(ETextLocation::BeginningOfCodeLine, ECursorAction::SelectText);
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorSelectLineEnd,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->JumpTo(ETextLocation::EndOfLine, ECursorAction::SelectText);
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorTokenSelectLeft,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(-1, 0),
					ECursorAction::SelectText
				));
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorTokenSelectRight,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(+1, 0),
					ECursorAction::SelectText
				));
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorTokenLeft,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(-1, 0),
					ECursorAction::MoveCursor
				));
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().CursorTokenRight,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
					ECursorMoveGranularity::Word,
					FIntPoint(+1, 0),
					ECursorAction::MoveCursor
				));
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().ScrollUp,
			FExecuteAction::CreateLambda([this] {
				float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
				FVector2D Offset = ShaderMultiLineEditableTextLayout->GetScrollOffset();
				ShaderMultiLineEditableTextLayout->SetScrollOffset({Offset.X, Offset.Y - LineHeight}, ShaderMultiLineEditableText->GetTickSpaceGeometry());
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().ScrollDown,
			FExecuteAction::CreateLambda([this] {
				float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
				FVector2D Offset = ShaderMultiLineEditableTextLayout->GetScrollOffset();
				ShaderMultiLineEditableTextLayout->SetScrollOffset({ Offset.X, Offset.Y + LineHeight }, ShaderMultiLineEditableText->GetTickSpaceGeometry());
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().JumpTop,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableText->GoTo(ETextLocation::BeginningOfDocument);
			})
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().JumpBottom,
			FExecuteAction::CreateLambda([this] {
				ShaderMultiLineEditableText->GoTo(ETextLocation::EndOfDocument);
			})
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().MoveLineUp,
			FExecuteAction::CreateLambda([this] {
				const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
				const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
        		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
				auto& Lines = ShaderMarshaller->TextLayout->GetLineModels();
				if (StartLineIndex <= 0)
				{
					return;
				}

				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);

				TArray<FTextLayout::FLineModel> MovedLines;
				for (int32 Index = EndLineIndex; Index >= StartLineIndex; Index--)
				{
					MovedLines.Add(Lines[Index]);
					ShaderMarshaller->TextLayout->RemoveLine(Index);
				}
				for (int32 Index = 0; Index < MovedLines.Num(); Index++)
				{
					FTextLayout::FTextOffsetLocations BeforeOffsetLocations;
					ShaderMarshaller->TextLayout->GetTextOffsetLocations(BeforeOffsetLocations);
					Lines.Insert(MovedLines[Index], StartLineIndex - 1);
					//TextLayout does not provide an API to insert a single line. 
					//To perform the insertion while preserving edit commands for an undo state, call OnAddLine() manually here.
					ShaderMarshaller->TextLayout->OnAddLine(StartLineIndex - 1, *MovedLines[Index].Text, BeforeOffsetLocations);
				}

				TArray<int32> AffectedBreakPoints;
				for (auto It = BreakPointLineNumbers.CreateIterator(); It; ++It)
				{
					int32 BreakPointLineIndex = GetLineIndex(*It);
					if (BreakPointLineIndex == StartLineIndex - 1)
					{
						AffectedBreakPoints.Add(GetLineNumber(EndLineIndex));
						It.RemoveCurrent();
					}
					else if (BreakPointLineIndex >= StartLineIndex && BreakPointLineIndex <= EndLineIndex)
					{
						AffectedBreakPoints.Add(GetLineNumber(BreakPointLineIndex - 1));
						It.RemoveCurrent();
					}
				}
				BreakPointLineNumbers.Append(AffectedBreakPoints);

				const FTextLocation NewSelectionStart(StartLineIndex - 1, Selection.GetBeginning().GetOffset());
				const FTextLocation NewSelectionEnd(EndLineIndex - 1, Selection.GetEnd().GetOffset());
				ShaderMultiLineEditableText->SelectText(NewSelectionStart, NewSelectionEnd);
			}),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().MoveLineDown,
			FExecuteAction::CreateLambda([this] {
				const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
				const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
        		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
				auto& Lines = ShaderMarshaller->TextLayout->GetLineModels();
				if (EndLineIndex >= Lines.Num() - 1)
				{
					return;
				}

				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);

				TArray<FTextLayout::FLineModel> MovedLines;
				for (int32 Index = EndLineIndex; Index >= StartLineIndex; Index--)
				{
					MovedLines.Add(Lines[Index]);
					ShaderMarshaller->TextLayout->RemoveLine(Index);
				}
				for (int32 Index = 0; Index < MovedLines.Num(); Index++)
				{
					FTextLayout::FTextOffsetLocations BeforeOffsetLocations;
					ShaderMarshaller->TextLayout->GetTextOffsetLocations(BeforeOffsetLocations);
					Lines.Insert(MovedLines[Index], StartLineIndex + 1);
					ShaderMarshaller->TextLayout->OnAddLine(StartLineIndex + 1, *MovedLines[Index].Text, BeforeOffsetLocations);
				}

				TArray<int32> AffectedBreakPoints;
				for (auto It = BreakPointLineNumbers.CreateIterator(); It; ++It)
				{
					int32 BreakPointLineIndex = GetLineIndex(*It);
					if (BreakPointLineIndex == EndLineIndex + 1)
					{
						AffectedBreakPoints.Add(GetLineNumber(StartLineIndex));
						It.RemoveCurrent();
					}
					else if (BreakPointLineIndex >= StartLineIndex && BreakPointLineIndex <= EndLineIndex)
					{
						AffectedBreakPoints.Add(GetLineNumber(BreakPointLineIndex + 1));
						It.RemoveCurrent();
					}
				}
				BreakPointLineNumbers.Append(AffectedBreakPoints);

				const FTextLocation NewSelectionStart(StartLineIndex + 1, Selection.GetBeginning().GetOffset());
				const FTextLocation NewSelectionEnd(EndLineIndex + 1, Selection.GetEnd().GetOffset());
				ShaderMultiLineEditableText->SelectText(NewSelectionStart, NewSelectionEnd);
			}),
			EUIActionRepeatMode::RepeatEnabled
		);

        FoldingArrowAnim.AddCurve(0, 0.25f, ECurveEaseFunction::Linear);

        //Hook the default FCursorLineHighlighter.
        TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>& CursorLineHighlighter = GetPrivate_FSlateEditableTextLayout_CursorLineHighlighter(*ShaderEditableTextLayout);
        SlateEditableTextTypes::FCursorInfo& CursorInfo = GetPrivate_FSlateEditableTextLayout_CursorInfo(*ShaderEditableTextLayout);
        
        CustomCursorHighlighter = CursorHighlighter::Create(&CursorInfo);
        CursorLineHighlighter = CustomCursorHighlighter;
        
        CurEditState = ShaderAssetObj->bCompilationSucceed ? EditState::Succeed : EditState::Failed;

		RefreshFont();
    }

	void SShaderEditorBox::ToggleComment()
	{
		const FTextLocation& CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
		const FTextSelection& Selection = ShaderMultiLineEditableText->GetSelection();
		const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		FTextLocation OldStart = Selection.GetBeginning();
		FTextLocation OldEnd = Selection.GetEnd();

		bool CancelComment = true;
		for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
		{
			FString LineText;
			ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);
			int32 Col = 0;
			while (Col < LineText.Len() && FChar::IsWhitespace(LineText[Col]))
			{
				++Col;
			}

			if (LineText.Mid(Col, 2) != TEXT("//"))
			{
				CancelComment = false;
			}
		}

		SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
		if (!CancelComment)
		{
			int32 FinalInserCol = std::numeric_limits<int32>::max();
			TMap<int32, bool> CanInsert;
			for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
			{
				FString LineText;
				ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);

				CanInsert.Add(LineIndex, false);

				int32 InsertCol = 0;
				do
				{
					if (InsertCol >= LineText.Len())
					{
						break;
					}
					if (!FChar::IsWhitespace(LineText[InsertCol]))
					{
						CanInsert[LineIndex] = true;
						break;
					}
					InsertCol++;
				} while (InsertCol);

				if (CanInsert[LineIndex]) {
					FinalInserCol = FMath::Min(FinalInserCol, InsertCol);
				}
			}
			FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
			if (FinalInserCol != std::numeric_limits<int32>::max())
			{
				for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
				{
					if (CanInsert[LineIndex])
					{
						ShaderTextLayout->InsertAt(FTextLocation{ LineIndex, FinalInserCol }, "// ");
						if (OldStart.GetLineIndex() == LineIndex && OldStart.GetOffset() >= FinalInserCol)
							OldStart = FTextLocation(LineIndex, OldStart.GetOffset() + 3);
						if (OldEnd.GetLineIndex() == LineIndex && OldEnd.GetOffset() >= FinalInserCol)
							OldEnd = FTextLocation(LineIndex, OldEnd.GetOffset() + 3);
					}
				}
				bool bCursorAtEnd = (OldEnd == CursorLocation);
				if (bCursorAtEnd)
				{
					ShaderMultiLineEditableText->SelectText(OldStart, OldEnd);
				}
				else
				{
					ShaderMultiLineEditableText->SelectText(OldEnd, OldStart);
				}
			}
		}
		else
		{
			for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; ++LineIndex)
			{
				FString LineText;
				ShaderMultiLineEditableText->GetTextLine(LineIndex, LineText);
				int32 RemoveCol = 0;
				while (RemoveCol < LineText.Len() && FChar::IsWhitespace(LineText[RemoveCol]))
				{
					++RemoveCol;
				}

				if (LineText.Mid(RemoveCol, 2) == TEXT("//"))
				{
					int32 RemoveLen = 2;
					if (LineText.Len() > RemoveCol + 2 && LineText[RemoveCol + 2] == TEXT(' '))
					{
						++RemoveLen;
					}
					FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
					ShaderTextLayout->RemoveAt(FTextLocation{ LineIndex, RemoveCol }, RemoveLen);

					if (OldStart.GetLineIndex() == LineIndex && OldStart.GetOffset() >= RemoveCol)
						OldStart = FTextLocation(OldStart, -FMath::Min(RemoveLen, OldStart.GetOffset() - RemoveCol));
					if (OldEnd.GetLineIndex() == LineIndex && OldEnd.GetOffset() >= RemoveCol)
						OldEnd = FTextLocation(OldEnd, -FMath::Min(RemoveLen, OldEnd.GetOffset() - RemoveCol));
				}
				bool bCursorAtEnd = (OldEnd == CursorLocation);
				if (bCursorAtEnd)
				{
					ShaderMultiLineEditableText->SelectText(OldStart, OldEnd);
				}
				else
				{
					ShaderMultiLineEditableText->SelectText(OldEnd, OldStart);
				}
			}
		}
	}

	void SShaderEditorBox::OnBeginEditTransaction()
	{
		SelectionBeforeEdit = ShaderMultiLineEditableText->GetSelection();
	}

	TRefCountPtr<GpuShader> SShaderEditorBox::CreateGpuShader()
	{
		return GGpuRhi->CreateShaderFromSource(ShaderAssetObj->GetShaderDesc(CurrentShaderSource));;
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
		FString Fmt = LOCALIZATION("Line").ToString() + ":{0} " + LOCALIZATION("Column").ToString() + ":{1}";
		return FText::FromString(FString::Format(*Fmt, { GetLineNumber(CursorRow), CursorCol }));
    }

    TSharedRef<SWidget> SShaderEditorBox::BuildInfoBar()
    {
        SAssignNew(InfoBarBox, SHorizontalBox);

		InfoBarBox->ClearChildren();

		FSlateFontInfo InforBarFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		InfoBarBox->AddSlot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsFocusable(false)
				.OnClicked_Lambda([this] {
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
			.Padding(2, 0, 0, 0)
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
				SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(this, &SShaderEditorBox::GetEditStateColor)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Font(InforBarFontInfo)
							.ColorAndOpacity(FLinearColor::Black)
							.Text(this, &SShaderEditorBox::GetRowColText)
							.Margin(FMargin{ 8, 0, 8, 0 })
					]
			];

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
		FString TabSpace;
		for (int i = 0; i < GetTabSize(); i++) { TabSpace += " "; }
		PastedText = PastedText.Replace(TEXT("\t"), *TabSpace);
        ShaderMultiLineEditableText->InsertTextAtCursor(PastedText);
        
        ShaderMarshaller->MakeDirty();
        ShaderMultiLineEditableText->Refresh();
    }

    void SShaderEditorBox::CopyText()
    {
        if (!ShaderMultiLineEditableTextLayout->AnyTextSelected())
        {
			int32 CursorLineIndex = ShaderMultiLineEditableTextLayout->GetCursorLocation().GetLineIndex();
			FString LineText;
			ShaderMultiLineEditableText->GetTextLine(CursorLineIndex, LineText);
			FTextSelection Selection = { { CursorLineIndex, 0 }, {CursorLineIndex, LineText.Len()} };
			FString UnFoldedText = UnFoldText(Selection);
			FPlatformApplicationMisc::ClipboardCopy(*UnFoldedText);
        }
		else
		{
			const FTextSelection Selection = ShaderMultiLineEditableTextLayout->GetSelection();
			FString SelectedText = UnFoldText(Selection);
			FPlatformApplicationMisc::ClipboardCopy(*SelectedText);
		}
		
    }

    void SShaderEditorBox::CutText()
    {
		SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);

        if (!ShaderMultiLineEditableTextLayout->AnyTextSelected())
        {
			int32 CursorLineIndex = ShaderMultiLineEditableTextLayout->GetCursorLocation().GetLineIndex();
			FString LineText;
			ShaderMultiLineEditableText->GetTextLine(CursorLineIndex, LineText);
			FTextLocation SelectionEnd{ CursorLineIndex, LineText.Len()};
			if (ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(CursorLineIndex + 1))
			{
				SelectionEnd = { CursorLineIndex + 1, 0 };
			}
			
			ShaderMultiLineEditableTextLayout->SelectText({ CursorLineIndex , 0}, SelectionEnd);
        }

		const FTextSelection Selection = ShaderMultiLineEditableTextLayout->GetSelection();
		FString SelectedText = UnFoldText(Selection);

		int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		for (int32 LineIndex = StartLineIndex; LineIndex <= EndLineIndex; LineIndex++)
		{
			RemoveFoldMarker(LineIndex);
		}

		FPlatformApplicationMisc::ClipboardCopy(*SelectedText);
		ShaderMultiLineEditableTextLayout->DeleteSelectedText();
		ShaderMultiLineEditableTextLayout->UpdateCursorHighlight();
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
		int32 OldLineCount = LineNumberData.Num() - PaddingLineNum;
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

		for(int i = 0; i < PaddingLineNum; i++)
		{
			LineNumberData.Add(MakeShared<FText>(FText::FromString("-")));
		}
		
		int32 DeltaLineCount = CurTextLayoutLine - OldLineCount;
		bool IsRedoOrUndo = ShaderMultiLineEditableTextLayout && ShaderMultiLineEditableTextLayout->CurrentUndoLevel >= 0;
		if(!IsFoldEditTransaction && !IsRedoOrUndo && DeltaLineCount != 0)
		{
			for(auto It = BreakPointLineNumbers.CreateIterator(); It; ++It)
			{
				if(StartLineNumber <= *It)
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
        return ShaderMultiLineEditableText->OnMouseWheel(ShaderMultiLineEditableText->GetTickSpaceGeometry(), MouseEvent);
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
        TUniquePtr<FSlateEditableTextLayout>& EffectEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*EffectMultiLineEditableText);
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);

		double EffectLayoutWidth = EffectEditableTextLayout->GetSize().X;
		FMargin CurrentMargin = ShaderEditableTextLayout->GetMargin();
		double ShaderLayoutWidth = ShaderEditableTextLayout->GetSize().X - CurrentMargin.Right;

        double XOffsetSizeBetweenLayout = FMath::Max(0.f, EffectLayoutWidth - ShaderLayoutWidth);
		//Ensure that ShaderMultiLineEditableText can accommodate the effect text.
		float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
        ShaderMultiLineEditableText->SetMargin(FMargin{ 0, 0, (float)XOffsetSizeBetweenLayout + 100, LineHeight * PaddingLineNum });

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
			for(const auto& [TokenRange, TokenType] : SyntaxHighlightMap)
			{
				for(auto& RunModel : LineModel.Runs)
				{
					TSharedRef<IRun> Run = RunModel.GetRun();
					if(Run->GetTextRange() == TokenRange)
					{
						RunModel = FTextLayout::FRunModel(FSlateTextStyleRefRun::Create(FRunInfo(), LineModel.Text, GetTokenStyleMap()[TokenType], TokenRange));
					}
				}
			}
		}
		
		GetPrivate_FTextLayout_DirtyFlags(*ShaderMarshaller->TextLayout) |= (1 << 0);
	}

	void SShaderEditorBox::RefreshOccurrenceHighlight()
	{
		int32 CursorLineIndex = ShaderMultiLineEditableText->GetCursorLocation().GetLineIndex();
		int32 CursorOffset = ShaderMultiLineEditableText->GetCursorLocation().GetOffset();
		if (ShaderMarshaller->TokenizedLines.IsValidIndex(CursorLineIndex))
		{
			int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();
			TArray<ShaderOccurrence> Occurrences;
			for (const HlslTokenizer::Token& Token : ShaderMarshaller->TokenizedLines[CursorLineIndex].Tokens)
			{
				if ((Token.Type == HLSL::TokenType::Identifier || Token.Type == HLSL::TokenType::BuildtinFunc)
					&& CursorOffset >= Token.BeginOffset && CursorOffset <= Token.EndOffset)
				{
					Occurrences = SyntaxTU.GetOccurrences(GetLineNumber(CursorLineIndex) + AddedLineNum, Token.BeginOffset + 1);
					break;
				}
			}
			for (const FTextLineHighlight& Highlight : OccurrenceHighlights)
			{
				if (ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(Highlight.LineIndex))
				{
					ShaderMarshaller->TextLayout->RemoveLineHighlight(Highlight);
				}
			}
			OccurrenceHighlights.Empty();
			for (const ShaderOccurrence& Occurrence : Occurrences)
			{
				int32 OccurrenceLineIndex = GetLineIndex(Occurrence.Row - AddedLineNum);
				if (ShaderMarshaller->TokenizedLines.IsValidIndex(OccurrenceLineIndex))
				{
					for (const HlslTokenizer::Token& Token : ShaderMarshaller->TokenizedLines[OccurrenceLineIndex].Tokens)
					{
						if (Token.BeginOffset == Occurrence.Col - 1)
						{
							//why -11? see FSlateEditableTextLayout::UpdateCursorHighlight() 
							OccurrenceHighlights.Emplace(OccurrenceLineIndex, FTextRange{ Token.BeginOffset, Token.EndOffset }, -11, OccurrenceHighlighter::Create());
							break;
						}
					}
				}

			}
			for (const FTextLineHighlight& Highlight : OccurrenceHighlights)
			{
				ShaderMarshaller->TextLayout->AddLineHighlight(Highlight);
			}
		}
	}

	void SShaderEditorBox::RefreshBracketHighlight()
	{
		int32 CursorLineIndex = ShaderMultiLineEditableText->GetCursorLocation().GetLineIndex();
		int32 CursorOffset = ShaderMultiLineEditableText->GetCursorLocation().GetOffset();
		if (ShaderMarshaller->TokenizedLines.IsValidIndex(CursorLineIndex))
		{
			if (OpenBracketHighlight)
			{
				if (ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(OpenBracketHighlight.GetValue().LineIndex))
				{
					ShaderMarshaller->TextLayout->RemoveLineHighlight(OpenBracketHighlight.GetValue());
				}
			}
			if (CloseBracketHighlight)
			{
				if (ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(CloseBracketHighlight.GetValue().LineIndex))
				{
					ShaderMarshaller->TextLayout->RemoveLineHighlight(CloseBracketHighlight.GetValue());
				}
			}
			OpenBracketHighlight.Reset();
			CloseBracketHighlight.Reset();
			for (const auto& BracketGroup : ShaderMarshaller->BracketGroups)
			{
				bool HasMatchedBracketGroup{};
				if (CursorLineIndex >= BracketGroup.OpenLineIndex && CursorLineIndex <= BracketGroup.CloseLineIndex)
				{
					if (BracketGroup.OpenLineIndex == BracketGroup.CloseLineIndex)
					{
						if (CursorOffset >= BracketGroup.OpenBracket.Offset && CursorOffset <= BracketGroup.CloseBracket.Offset + 1)
						{
							HasMatchedBracketGroup = true;
						}
					}
					else if (CursorLineIndex == BracketGroup.OpenLineIndex)
					{
						if (CursorOffset >= BracketGroup.OpenBracket.Offset)
						{
							HasMatchedBracketGroup = true;
						}
					}
					else if (CursorLineIndex == BracketGroup.CloseLineIndex)
					{
						if (CursorOffset <= BracketGroup.CloseBracket.Offset + 1)
						{
							HasMatchedBracketGroup = true;
						}
					}
					else
					{
						HasMatchedBracketGroup = true;
					}
				}

				bool UpdateBracketHighlight{};
				if (HasMatchedBracketGroup)
				{
					if (OpenBracketHighlight)
					{
						if (BracketGroup.OpenLineIndex > OpenBracketHighlight.GetValue().LineIndex)
						{
							UpdateBracketHighlight = true;
						}
						else if (BracketGroup.OpenLineIndex == OpenBracketHighlight.GetValue().LineIndex && BracketGroup.OpenBracket.Offset > OpenBracketHighlight.GetValue().Range.BeginIndex)
						{
							UpdateBracketHighlight = true;
						}
					}
					else
					{
						UpdateBracketHighlight = true;
					}
				}

				if (UpdateBracketHighlight)
				{
					OpenBracketHighlight = { BracketGroup.OpenLineIndex, FTextRange{ BracketGroup.OpenBracket.Offset, BracketGroup.OpenBracket.Offset + 1 }, -11, BracketHighlighter::Create() };
					CloseBracketHighlight = { BracketGroup.CloseLineIndex, FTextRange{ BracketGroup.CloseBracket.Offset, BracketGroup.CloseBracket.Offset + 1 }, -11, BracketHighlighter::Create() };
				}

			}

			if (OpenBracketHighlight && ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(OpenBracketHighlight.GetValue().LineIndex))
			{
				ShaderMarshaller->TextLayout->AddLineHighlight(OpenBracketHighlight.GetValue());
			}
			if (CloseBracketHighlight && ShaderMarshaller->TextLayout->GetLineModels().IsValidIndex(CloseBracketHighlight.GetValue().LineIndex))
			{
				ShaderMarshaller->TextLayout->AddLineHighlight(CloseBracketHighlight.GetValue());
			}

		}
	}

    void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
		//ShaderMultiLineEditableText updates its VScrollBar when it ticks, and we sync the LineNumberList/LineTipList according to its VScrollBar callback.
		//Which leads to a delay of one frame in the drawing of the list, so here we tick it earlier.
		ShaderMultiLineEditableTextLayout->Tick(ShaderMultiLineEditableText->GetTickSpaceGeometry(), InCurrentTime, InDeltaTime);
        
		if (bRefreshIsense.load(std::memory_order_acquire))
		{
			RefreshLineNumberToDiagInfo();
			RefreshCodeCompletionTip();
			bRefreshIsense.store(false, std::memory_order_relaxed);
		}

		if (bRefreshSyntax.load(std::memory_order_acquire))
		{
			RefreshSyntaxHighlight();
			bRefreshSyntax.store(false, std::memory_order_relaxed);
		}

        UpdateEffectText();
    }

	void SShaderEditorBox::OnCursorMoved(const FTextLocation& InLocation)
	{
		if(!bKeyChar) {
			bTryComplete = false;
			bTryMergeUndoState = false;
			RefreshOccurrenceHighlight();
		}
		bKeyChar = false;
		RefreshBracketHighlight();
	}

	void SShaderEditorBox::OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
    {
        bTryComplete = false;
		bTryMergeUndoState = false;

		bool bContainsSelf = false;
		for (int32 i = 0; i < NewWidgetPath.Widgets.Num(); ++i)
		{
			if (NewWidgetPath.Widgets[i].Widget == AsShared())
			{
				bContainsSelf = true;
				break;
			}
		}
		if (!bContainsSelf)
		{
			ShaderMarshaller->TextLayout->ClearLineHighlights();
		}
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
		const auto& CodeFontInfo = GetCodeFontInfo();
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
		
        auto Row = SNew(STableRow<CandidateItemPtr>, OwnerTable)
		.MouseButtonDownFocusWidget(ShaderMultiLineEditableText)
		.DetectDrag(false)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            [
				CandidateText
            ]
            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock).Font(CodeFontInfo)
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
		for (const auto& CandidateInfo : CandidateInfos)
		{
			CandidateItems.Add(MakeShared<ShaderCandidateInfo>(CandidateInfo));
			if (CandidateItems.Num() >= 40)
			{
				break;
			}
		}

		if (CandidateItems.Num() > 0)
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
            int32 DiagInfoLineNumber = (int32)DiagInfo.Row - ShaderAssetObj->GetExtraLineNum();
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
		TRefCountPtr<GpuShader> Shader = CreateGpuShader();
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
        FString NewShaderSource = InShaderSouce.Replace(TEXT("\r\n"), TEXT("\n"));
        if (NewShaderSource != ShaderAssetObj->EditorContent)
        {
			if(Debugger.IsValid())
			{
				bEditDuringDebugging = true;
			}
			CurEditState = EditState::Editing;
			ShaderAssetObj->EditorContent = NewShaderSource;
			ShaderAssetObj->MarkDirty(NewShaderSource != ShaderAssetObj->SavedEditorContent);
        }
        
        CurrentShaderSource = NewShaderSource;

		ISenseTask Task{};
		Task.ShaderDesc = ShaderAssetObj->GetShaderDesc(CurrentShaderSource);

		if (bTryComplete)
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
			if (TokenizedLines[0].Tokens.Num() > 1)
			{
				Token = TokenizedLines[0].Tokens.Last(1);
				FString LeftToken2 = CursorLeft.Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
				if (LeftToken2 == ".")
				{
					Task.IsMemberAccess = true;
				}
			}
			CurToken = LeftToken;

			Task.Row = GetLineNumber(CursorRow) + AddedLineNum;
			if (LeftToken == ".")
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
		
		const FTextSelection& Selection = ShaderMultiLineEditableText->GetSelection();
		const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		
		const FTextLocation& CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
		const int32 CursorLine = CursorLocation.GetLineIndex();
		const int32 CursorOffset = CursorLocation.GetOffset();

		if (UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
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

		if (Key == EKeys::Left)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				FIntPoint(-1, 0),
				ECursorAction::MoveCursor
			));
			return FReply::Handled();
		}
		else if (Key == EKeys::Right)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				FIntPoint(+1, 0),
				ECursorAction::MoveCursor
			));
			return FReply::Handled();
		}
		else if (Key == EKeys::Up)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				// Move up
				FIntPoint(0, -1),
				// Shift selects text.	
				InKeyEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
			));
			return FReply::Handled();
		}
		else if (Key == EKeys::Down)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				// Move down
				FIntPoint(0, +1),
				// Shift selects text.	
				InKeyEvent.IsShiftDown() ? ECursorAction::SelectText : ECursorAction::MoveCursor
			));
			return FReply::Handled();
		}

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
			//Ignore FSlateEditableTextLayout::HandleKeyChar backspace
			return FReply::Handled();
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
				const int32 TabSize = GetTabSize();

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
                        const int32 NumExtraSpaces = NumSpaces % TabSize;

                        // Tab to nearest TabSize.
                        int32 NumSpacesForIndentation;
                        if (bShouldIncreaseIndentation)
                        {
                            NumSpacesForIndentation = NumExtraSpaces == 0 ? TabSize : TabSize - NumExtraSpaces;
                            Text->InsertTextAtCursor(FString::ChrN(NumSpacesForIndentation, TEXT(' ')));
                        }
                        else
                        {
                            NumSpacesForIndentation = NumExtraSpaces == 0 ? FMath::Min(TabSize, NumSpaces) : NumExtraSpaces;
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

                    // Tab to nearest TabSize.
                    if (ensure(bShouldIncreaseIndentation))
                    {
                        const int32 NumSpacesForIndentation = TabSize - Offset % TabSize;
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

	void SShaderEditorBox::UnFold(int32 LineNumber)
	{
		int32 LineIndex = GetLineIndex(LineNumber);
		if(LineIndex == INDEX_NONE)
		{
			for(const FoldMarker& Marker : VisibleFoldMarkers)
			{
				int StartLineNumber = GetLineNumber(Marker.RelativeLineIndex);
				int EndLineNumber = StartLineNumber + Marker.GetFoldedLineCounts();
				if(LineNumber >= StartLineNumber && LineNumber <= EndLineNumber)
				{
					LineIndex = Marker.RelativeLineIndex;
					break;
				}
			}
		}

		TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
		if(MarkerIndex)
		{
			IsFoldEditTransaction = true;
			ShaderMultiLineEditableTextLayout->BeginEditTransation();

			FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
			TArray< FTextLayout::FLineModel >& Lines = ShaderTextLayout->GetLineModels();
			const FTextLocation& CurCursorLocation = ShaderMultiLineEditableText->GetCursorLocation();

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
				if (i == FoldedLineText.Num() - 1)
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

			ShaderMultiLineEditableTextLayout->EndEditTransaction();
			IsFoldEditTransaction = false;

			if(GetLineIndex(LineNumber) == INDEX_NONE)
			{
				UnFold(LineNumber);
			}
		}
	}

	void SShaderEditorBox::Fold(int32 LineNumber)
	{
		IsFoldEditTransaction = true;
		ShaderMultiLineEditableTextLayout->BeginEditTransation();

		int32 LineIndex = GetLineIndex(LineNumber);
		FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
		TArray< FTextLayout::FLineModel >& Lines = ShaderTextLayout->GetLineModels();
		const FTextLocation& CurCursorLocation = ShaderMultiLineEditableText->GetCursorLocation();

		auto BraceGroup = ShaderMarshaller->FoldingBraceGroups[LineIndex];
		int32 FoldedBeginningRow = BraceGroup.OpenLineIndex;
		int32 FoldedBeginningCol = BraceGroup.OpenBracket.Offset;
		int32 FoldedEndRow = BraceGroup.CloseLineIndex;
		int32 FoldedEndCol = BraceGroup.CloseBracket.Offset;

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

		const FTextLocation NewCursorLocation(FoldedBeginningRow, BeginLine.Text->Len());
		ShaderMultiLineEditableText->GoTo(NewCursorLocation);

		ShaderMultiLineEditableTextLayout->EndEditTransaction();
		IsFoldEditTransaction = false;
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
        TOptional<int32> MarkerIndex = FindFoldMarker(LineIndex);
    
        if (!MarkerIndex)
        {
			Fold(LineNumber);
        }
        else
        {
			UnFold(LineNumber);
        }
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
		float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
		auto LineHeightBox = SNew(SBox).HeightOverride(LineHeight);
		auto LineNumberRow = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
			.Content()
			[
				LineHeightBox
			];

		if (Item->ToString() == "-")
		{
			return LineNumberRow;
		}

        int32 LineNumber = FCString::Atoi(*(*Item).ToString());
        const int32 LineIndex = GetLineIndex(LineNumber);
		
		const auto& CodeFontInfo = GetCodeFontInfo();
		float MinWidth = (float)FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("0"), CodeFontInfo).X;
		MinWidth *= FString::FromInt(MaxLineNumber).Len();
        
        TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
            .Font(CodeFontInfo)
            .ColorAndOpacity_Lambda([this, LineIndex]{
                const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
                const int32 BeginLineIndex = Selection.GetBeginning().GetLineIndex();
                const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
                if(LineIndex >= BeginLineIndex && LineIndex <= EndLineIndex && ShaderMultiLineEditableText->HasKeyboardFocus())
                {
                    return HighlightLineNumberTextColor;
                }
                return NormalLineNumberTextColor;
            })
            .Text(*Item)
            .Justification(ETextJustify::Right)
            .MinDesiredWidth(MinWidth);
		
		LineNumberTextBlock->SetOnMouseButtonDown(FPointerEventHandler::CreateLambda([this, LineNumber](const FGeometry&, const FPointerEvent&){
			if(BreakPointLineNumbers.Contains(LineNumber))
			{
				BreakPointLineNumbers.Remove(LineNumber);
			}
			else{
				BreakPointLineNumbers.Add(LineNumber);
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
		.Image_Lambda([=, this] {
			if(IsWarningLine(LineNumber) || IsErrorLine(LineNumber))
			{
				return FAppStyle::Get().GetBrush("Icons.Warning");
			}
			else if(Debugger.GetStopLineNumber() == LineNumber)
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
			else if(Debugger.GetStopLineNumber() == LineNumber && Debugger.GetDebuggerError().IsEmpty())
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
			else if(BreakPointLineNumbers.Contains(LineNumber) || Debugger.GetStopLineNumber() == LineNumber)
			{
				return EVisibility::HitTestInvisible;
			}
			return EVisibility::Hidden;
		});
		
		
		LineHeightBox->SetContent(
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride_Lambda([this] { return GetFontSize() + 2; })
						.HeightOverride_Lambda([this] { return GetFontSize() + 2; })
						[
							SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
							[
								LineMarker
							]
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
						SNew(SBox)
						.WidthOverride_Lambda([this] { return GetFontSize(); })
						.HeightOverride_Lambda([this] { return GetFontSize(); })
						[
							SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
							[
								FoldingArrow.ToSharedRef()
							]
						]
					]
				]
				+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(1.0)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLineNumbers.Contains(LineNumber) || Debugger.GetStopLineNumber() == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber]{
							if(Debugger.GetStopLineNumber() == LineNumber && Debugger.GetDebuggerError().IsEmpty())
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
						if(BreakPointLineNumbers.Contains(LineNumber) || Debugger.GetStopLineNumber() == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.Image(FAppStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda([this, LineNumber]{
						if(Debugger.GetStopLineNumber() == LineNumber && Debugger.GetDebuggerError().IsEmpty())
						{
							return FLinearColor{0,1,0,0.06f};
						}
						return FLinearColor{1,0,0,0.06f};
					})
				]
		);
        
		return LineNumberRow;
    }

    TSharedRef<ITableRow> SShaderEditorBox::GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
    {
		float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
		auto LineHeightBox = SNew(SBox).HeightOverride(LineHeight);
		auto LineTip = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineTipItemStyle"))
			.Content()
			[
				LineHeightBox
			];

		//Dummy
		if (Item->ToString() == "-")
		{
			return SNew(STableRow<LineNumberItemPtr>, OwnerTable).Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
				.Content()[LineHeightBox];
		}

		int32 LineNumber = FCString::Atoi(*(*Item).ToString());
		
		LineTip->SetContent(
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					LineHeightBox
				]
				+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(1.0)
					.Visibility_Lambda([this, LineNumber]{
						if(BreakPointLineNumbers.Contains(LineNumber) || Debugger.GetStopLineNumber() == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber]{
							if(Debugger.GetStopLineNumber() == LineNumber && Debugger.GetDebuggerError().IsEmpty())
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
						if(BreakPointLineNumbers.Contains(LineNumber) || Debugger.GetStopLineNumber() == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.BorderImage_Lambda([this, LineNumber]{
						if(Debugger.GetStopLineNumber() == LineNumber && !Debugger.GetDebuggerError().IsEmpty())
						{
							return FAppStyle::Get().GetBrush("WhiteBrush");
						}
						return FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect");
					})
					.BorderBackgroundColor_Lambda([this, LineNumber]{
						if(Debugger.GetStopLineNumber() == LineNumber && Debugger.GetDebuggerError().IsEmpty())
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
						.Visibility_Lambda([this, LineNumber] { return Debugger.GetStopLineNumber() == LineNumber && !Debugger.GetDebuggerError().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
						.Text_Lambda([this] { return FText::FromString(Debugger.GetDebuggerError()); })
						.ColorAndOpacity(FLinearColor::Red)
					]
				]
        );

    
        LineTip->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, LineNumber]{
            const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
            const int32 CurLineIndex = CursorLocation.GetLineIndex();
            auto FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
            if(FocusedWidget == ShaderMultiLineEditableText && LineNumber == GetLineNumber(CurLineIndex)
			   && !BreakPointLineNumbers.Contains(LineNumber) && Debugger.GetStopLineNumber() != LineNumber)
            {
                double CurTime = FPlatformTime::Seconds();
                float Speed = 2.0f;
                double AnimatedOpacity = (HighlightLineTipColor.A - 0.1f) * FMath::Pow(FMath::Abs(FMath::Sin(CurTime * Speed)),1.8) + 0.1f;
                return FLinearColor{ HighlightLineTipColor.R, HighlightLineTipColor.G, HighlightLineTipColor.B, (float)AnimatedOpacity };
            }
            return NormalLineTipColor;
        }));
        
        return LineTip;
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
						FTextBlockStyle* RunTextStyle = &OwnerWidget->GetTokenStyleMap()[Token.Type];
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
								for(const auto& [TokenRange, TokenType] : OwnerWidget->LineSyntaxHighlightMapsCopy[LineIndex])
								{
									if(TokenRange.BeginIndex == MatchedToken->BeginOffset)
									{
										RunTextStyle = &OwnerWidget->GetTokenStyleMap()[TokenType];
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

		this->TokenizedLines.Empty();
		SyntaxTask Task;
		Task.ShaderDesc = OwnerWidget->GetShaderAsset()->GetShaderDesc(SourceString);
		Task.LineTokens.SetNum(LineModels.Num());
		for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
		{
			HlslTokenizer::TokenizedLine* CurTokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
			this->TokenizedLines.Add(*CurTokenizedLine);
			for (const HlslTokenizer::Token& Token : CurTokenizedLine->Tokens)
			{
				Task.LineTokens[LineIndex].Add(Token);
			}
		}
		OwnerWidget->bRefreshSyntax.store(false, std::memory_order_relaxed);
		OwnerWidget->SyntaxQueue.Enqueue(MoveTemp(Task));
		OwnerWidget->SyntaxEvent->Trigger();

		struct BracketStackData
		{
			int32 LineIndex{};
			HlslTokenizer::Bracket Bracket;
		};
        TArray<HlslTokenizer::BracketGroup> BraceGroups, ParenGroups;
		TArray<BracketStackData> OpenBraceStack, OpenParenStack;
        for(int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
        {
			HlslTokenizer::TokenizedLine* TokenizedLine = static_cast<HlslTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
            for(const HlslTokenizer::Bracket& Brace : TokenizedLine->Braces)
            {
                if(Brace.Type == HlslTokenizer::SideType::Open)
                {
                    OpenBraceStack.Emplace(LineIndex, Brace);
                }
				else
				{
					if(!OpenBraceStack.IsEmpty())
					{
						auto OpenBraceData = OpenBraceStack.Pop();
						BraceGroups.Emplace(OpenBraceData.LineIndex, OpenBraceData.Bracket, LineIndex, Brace);
					}
				}
            }
			for(const HlslTokenizer::Bracket& Paren : TokenizedLine->Parens)
			{
				if(Paren.Type == HlslTokenizer::SideType::Open)
				{
					OpenParenStack.Emplace(LineIndex, Paren);
				}
				else
				{
					if (!OpenParenStack.IsEmpty())
					{
						auto OpenParenData = OpenParenStack.Pop();
						ParenGroups.Emplace(OpenParenData.LineIndex, OpenParenData.Bracket, LineIndex, Paren);
					}
				}
			}
        }
		BracketGroups.Empty();
		BracketGroups.Append(BraceGroups);
		BracketGroups.Append(ParenGroups);

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

	void SShaderMultiLineEditableText::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TSharedPtr<SWindow> ShaderEditorTipWindow = ShEditor->GetShaderEditorTipWindow();

		FVector2D ScreenSpaceCursorPos = FSlateApplication::Get().GetCursorPos();
		FGeometry WindowGeometry = ShaderEditorTipWindow->GetWindowGeometryInScreen();
		bool bMouseInTipWindow = ShaderEditorTipWindow->IsVisible() && WindowGeometry.IsUnderLocation(ScreenSpaceCursorPos);

		static double LastCheckTime = 0.0;
		const double CheckInterval = 0.4;

		bool bShouldCheck = InCurrentTime - LastCheckTime >= CheckInterval;
		bShouldCheck = bShouldCheck && !bMouseInTipWindow;

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
				if (Owner->Debugger.IsValid())
				{
					int32 ExtraLineNum = Owner->GetShaderAsset()->GetExtraLineNum();
					const auto& TokenizedLine = Owner->ShaderMarshaller->TokenizedLines[CurrentHoverLocation.GetLineIndex()];
					FString CurTextLine;
					GetTextLine(CurrentHoverLocation.GetLineIndex(), CurTextLine);

					bool bFollowedBySquareBracket{};
					for (const auto& Token : TokenizedLine.Tokens)
					{
						FString TokenName = CurTextLine.Mid(Token.BeginOffset, Token.EndOffset - Token.BeginOffset);
						if (CurrentHoverLocation.GetOffset() >= Token.BeginOffset && CurrentHoverLocation.GetOffset() <= Token.EndOffset)
						{
							if (Token.Type == HLSL::TokenType::Identifier || TokenName == "]")
							{
								CurrentTokenName = MoveTemp(TokenName);
								CurrentTokenBeginOffset = Token.BeginOffset;
								CurrentTokenEndOffset = Token.EndOffset;
								if (CurTextLine.Mid(Token.EndOffset, 1) == "]")
								{
									bFollowedBySquareBracket = true;
								}
								break;
							}
						}
					}

					if (!CurrentTokenName.IsEmpty())
					{
						//Extract expression
						int32 BeginOffset = CurrentTokenBeginOffset;
						while (BeginOffset > 0)
						{
							TCHAR PrevChar = CurTextLine[BeginOffset - 1];
							if (FChar::IsIdentifier(PrevChar) || PrevChar == TEXT('.') ||
								PrevChar == TEXT('[') || PrevChar == TEXT(']'))
							{
								//If hovering between [ and ]
								if (bFollowedBySquareBracket && PrevChar == TEXT('['))
								{
									break;
								}
								BeginOffset--;
							}
							else
							{
								break;
							}
						}
		
						FString Expression = CurTextLine.Mid(BeginOffset, CurrentTokenEndOffset - BeginOffset);
						if (LastHoverExpr && LastHoverExpr->Expr == Expression)
						{
							HoverExpr =  LastHoverExpr;
						}
						else
						{
							ExpressionNode EvalResult = Owner->Debugger.EvaluateExpression(Expression);
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
					R"(\(\s*)"                                                           // (
					R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"         // X
					R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*,\s*)"         // Y
					R"(([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?)\s*)"             // z
					R"((?:\s*,\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?))?)"    // optional w
					R"(\s*\))"                                                           // )
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
										.Color(FLinearColor{ X, Y, Z, W})
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
														FString NewColorStr;
														if (HasAlpha)
															NewColorStr = FString::Printf(TEXT("(%.3f, %.3f, %.3f, %.3f)"), InColor.R, InColor.G, InColor.B, InColor.A);
														else
															NewColorStr = FString::Printf(TEXT("(%.3f, %.3f, %.3f)"), InColor.R, InColor.G, InColor.B);

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

		//Show the debug value when hovering over variables
		if (bShouldCheck)
		{
			if (!CheckDebuggerTip() && !CheckColorBlockTip())
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

	void SShaderEditorBox::ScrollTo(int32 InLineIndex)
	{
		int32 LineCount = ShaderMarshaller->TextLayout->GetLineCount();
		int32 VisibleLineCount = ShaderMarshaller->TextLayout->GetVisibleLineCount();
		int32 StartVisibleLineIndex = ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
		int32 EndVisibleLineIndex = ShaderMarshaller->TextLayout->GetEndVisibleLineIndex();
		if(InLineIndex <= StartVisibleLineIndex)
		{
			int32 TargetLineIndex = FMath::Clamp(InLineIndex - VisibleLineCount / 2, 0, LineCount - 1);
			ShaderMultiLineEditableText->ScrollTo({TargetLineIndex, 0});
		}
		else if(InLineIndex >= EndVisibleLineIndex)
		{
			int32 TargetLineIndex = FMath::Clamp(InLineIndex + VisibleLineCount / 2, 0, LineCount - 1);
			ShaderMultiLineEditableText->ScrollTo({TargetLineIndex, 0});
		}
	}

	void SShaderEditorBox::Continue(StepMode Mode)
	{
		AssetOp::OpenAsset(ShaderAssetObj);
		if(bEditDuringDebugging)
		{
			auto Ret = MessageDialog::Open(MessageDialog::OkCancel, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("EditDuringDebugging"));
			if(Ret == MessageDialog::MessageRet::Ok)
			{
				bEditDuringDebugging = false;
			}
			else
			{
				return;
			}
		}
		
		if(Debugger.Continue(Mode))
		{
			Debugger.ShowDebuggerResult();
			ScrollTo(GetLineIndex(Debugger.GetStopLineNumber()));
			UnFold(Debugger.GetStopLineNumber());
		}
		else
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->EndDebugging();
		}
	}

	void SShaderEditorBox::ResetDebugger()
	{
		bEditDuringDebugging = false;
		Debugger.Reset();
	}

	void SShaderEditorBox::DebugPixel(const BindingState& InBuilders, const PixelState& InState)
	{
		try
		{
			Debugger.DebugPixel(InBuilders, InState);
			Continue();
		}
		catch (const std::runtime_error& e)
		{
			MessageDialog::Open(MessageDialog::Ok, GApp->GetEditor()->GetMainWindow(), FText::FromString(UTF8_TO_TCHAR(e.what())));
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->EndDebugging();
			return;
		}
	}
}
