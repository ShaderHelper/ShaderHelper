#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include "AssetObject/Shader.h"
#include "GpuApi/GpuRhi.h"
#include "ShaderCodeEditorLineHighlighter.h"
#include "Editor/CodeEditorCommands.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "Common/Path/BaseResourcePath.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "Editor/PreviewViewPort.h"

#include <Widgets/Text/SlateEditableTextLayout.h>
#include <Widgets/Layout/SScrollBarTrack.h>
#include <Framework/Text/SlateTextRun.h>
#include <Widgets/Layout/SScaleBox.h>
#include <Framework/Commands/GenericCommands.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Framework/Text/TextLayout.h>
#include <Widgets/Text/SlateTextBlockLayout.h>
#include <Framework/Text/SlateTextHighlightRunRenderer.h>
#include <Fonts/FontMeasure.h>
#include <Styling/StyleColors.h>
#include <Widgets/SViewport.h>

#include <regex>

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SMultiLineEditableText, TUniquePtr<FSlateEditableTextLayout>, EditableTextLayout)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>, CursorLineHighlighter)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, SlateEditableTextTypes::FCursorInfo, CursorInfo)
STEAL_PRIVATE_MEMBER(FSlateEditableTextLayout, TOptional<SlateEditableTextTypes::FScrollInfo>, PositionToScrollIntoView)
STEAL_PRIVATE_MEMBER(STextBlock, TUniquePtr< FSlateTextBlockLayout >, TextLayoutCache)
STEAL_PRIVATE_MEMBER(FSlateTextBlockLayout, TSharedPtr<FSlateTextLayout>, TextLayout)
STEAL_PRIVATE_MEMBER(FTextLayout, uint8, DirtyFlags)

using namespace FW;

namespace SH
{

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

	SShaderEditorBox::SShaderEditorBox()
	{
	}

	SShaderEditorBox::~SShaderEditorBox()
    {
		ShaderAssetObj->OnShaderRefreshed.Remove(ShaderRefreshHandle);

		bQuitLang = true;
		LangEvent->Trigger();
		LangThread->Join();
		FPlatformProcess::ReturnSynchEventToPool(LangEvent);

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TSharedPtr<SWindow> ShaderEditorTipWindow = ShEditor->GetShaderEditorTipWindow();
		ShaderEditorTipWindow->SetContent(SNullWidget::NullWidget);
		ShaderEditorTipWindow->HideWindow();
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

	bool SShaderEditorBox::CanShowGuideLine()
	{
		bool ShowGuideLine = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("ShowGuideLine"), ShowGuideLine);
		return ShowGuideLine;
	}

	bool SShaderEditorBox::CanShowQuickInfo()
	{
		bool ShowQuickInfo = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("ShowQuickInfo"), ShowQuickInfo);
		return ShowQuickInfo;
	}

	bool SShaderEditorBox::CanRealTimeDiagnosis()
	{
		bool RealTimeDiagnosis = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("RealTimeDiagnosis"), RealTimeDiagnosis);
		return RealTimeDiagnosis;
	}

	bool SShaderEditorBox::CanHighlightCursorLine()
	{
		bool HighlightCursorLine = true;
		Editor::GetEditorConfig()->GetBool(TEXT("CodeEditor"), TEXT("HighlightCursorLine"), HighlightCursorLine);
		return HighlightCursorLine;
	}

	FSlateFontInfo& SShaderEditorBox::GetCodeFontInfo()
	{
		static FSlateFontInfo CodeFontInfo;
		return CodeFontInfo;
	}

	void SShaderEditorBox::RefreshFont()
	{
		// Save the current visible line index before changing font
		int32 StartVisibleLineIndex = ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
		double X = ShaderMultiLineEditableTextLayout->GetScrollOffset().X;

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
		for (auto& [_, Style] : GetDimTokenStyleMap())
		{
			Style.SetFont(CodeFontInfo);
		}
		ShaderMultiLineEditableText->SetFont(CodeFontInfo);
		//After marking Marshaller as dirty, calling refresh directly will rehandle the entire text, 
		//which may cause stuttering. Therefore, we only update layout
		ShaderMarshaller->ClearDirty();
		ShaderMarshaller->TextLayout->ResetMaxDrawWidth();
		ShaderMarshaller->TextLayout->UpdateIfNeeded();
		// Calculate Y offset using the new line height but the saved line index
		double Y = ShaderMarshaller->TextLayout->GetUniformLineHeight() * StartVisibleLineIndex / ShaderMarshaller->TextLayout->GetScale();
		ShaderMultiLineEditableTextLayout->SetScrollOffset({ X,Y }, ShaderMultiLineEditableText->GetTickSpaceGeometry());
		ShaderMultiLineEditableTextLayout->Tick(ShaderMultiLineEditableText->GetTickSpaceGeometry(), 0, 0);

		EffectMultiLineEditableText->SetFont(CodeFontInfo);
		EffectMultiLineEditableText->Refresh();

		LineNumberList->RebuildList();
		LineTipList->RebuildList();

	}

	const TCHAR* GetTokenStyleName(ShaderTokenType Type)
	{
		switch (Type)
		{
		case ShaderTokenType::Keyword:     return TEXT("CodeEditorKeywordText");
		case ShaderTokenType::BuiltinFunc: return TEXT("CodeEditorBuiltinFuncText");
		case ShaderTokenType::BuiltinType: return TEXT("CodeEditorBuiltinTypeText");
		case ShaderTokenType::Func:        return TEXT("CodeEditorFuncText");
		case ShaderTokenType::Type:        return TEXT("CodeEditorTypeText");
		case ShaderTokenType::Param:       return TEXT("CodeEditorParamText");
		case ShaderTokenType::Var:         return TEXT("CodeEditorVarText");
		case ShaderTokenType::Punctuation: return TEXT("CodeEditorPunctuationText");
		case ShaderTokenType::Preprocess:  return TEXT("CodeEditorPreprocessText");
		case ShaderTokenType::Number:      return TEXT("CodeEditorNumberText");
		case ShaderTokenType::Comment:     return TEXT("CodeEditorCommentText");
		case ShaderTokenType::String:      return TEXT("CodeEditorStringText");
		default:                           return TEXT("CodeEditorNormalText");
		}
	}

	TMap<ShaderTokenType, FTextBlockStyle>& SShaderEditorBox::GetTokenStyleMap()
	{
		static TMap<ShaderTokenType, FTextBlockStyle> TokenStyleMap = []() {
			TMap<ShaderTokenType, FTextBlockStyle> Map;
			for (ShaderTokenType Type : magic_enum::enum_values<ShaderTokenType>())
			{
				Map.Add(Type, FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>(GetTokenStyleName(Type)));
			}
			return Map;
		}();
		return TokenStyleMap;
	}

	static TMap<ShaderTokenType, FTextBlockStyle> DimTokenStyleMap;

	void SShaderEditorBox::RefreshDimTokenStyleMap()
	{
		for (ShaderTokenType Type : magic_enum::enum_values<ShaderTokenType>())
		{
			FTextBlockStyle Style = FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>(GetTokenStyleName(Type));
			FLinearColor BaseColor = Style.ColorAndOpacity.GetSpecifiedColor();
			if (FTextBlockStyle* ExistingStyle = DimTokenStyleMap.Find(Type))
			{
				ExistingStyle->SetColorAndOpacity(BaseColor.CopyWithNewOpacity(0.4f));
			}
			else
			{
				Style.SetColorAndOpacity(BaseColor.CopyWithNewOpacity(0.4f));
				DimTokenStyleMap.Add(Type, MoveTemp(Style));
			}
		}
	}

	TMap<ShaderTokenType, FTextBlockStyle>& SShaderEditorBox::GetDimTokenStyleMap()
	{
		if (DimTokenStyleMap.IsEmpty())
		{
			RefreshDimTokenStyleMap();
		}
		return DimTokenStyleMap;
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
		ShaderRefreshHandle = ShaderAssetObj->OnShaderRefreshed.AddLambda([this]{
			SyncFromShaderAsset();
			SubmitLangServiceTasks();
		});

		UICommandList = MakeShared<FUICommandList>();

		InitializeScrollBars();
		InitializeMarshalling();
		FText InitialShaderText = InitializeEditorSource();

		StartLangServiceThreads();
		BuildEditorWidget(InitialShaderText);
		ConfigureEditableTextLayout();
		BindCommands();

        FoldingArrowAnim.AddCurve(0, 0.25f, ECurveEaseFunction::Linear);
		InitializeCursorHighlighter();

        CurEditState = !ShaderAssetObj->IsCompilable() || ShaderAssetObj->IsCompilationSuccessful() ? EditState::Succeed : EditState::Failed;

		RefreshFont();
		SyncStageSelectorOptions(ShaderAssetObj->GetEnabledStageList());
    }

	FText SShaderEditorBox::InitializeEditorSource()
	{
		CurrentEditorSource = ShaderAssetObj->EditorContent;

		FString TabSpace;
		for (int i = 0; i < GetTabSize(); i++) { TabSpace += " "; }
		CurrentEditorSource = CurrentEditorSource.Replace(TEXT("\t"), *TabSpace);
		CurrentShaderSource = CurrentEditorSource.Replace(TEXT("\r\n"), TEXT("\n"));
		return FText::FromString(CurrentEditorSource);
	}

	void SShaderEditorBox::SyncFromShaderAsset()
	{
		if (!ShaderAssetObj)
		{
			return;
		}

		FString NewEditorSource = ShaderAssetObj->EditorContent;
		FString TabSpace;
		for (int i = 0; i < GetTabSize(); i++) { TabSpace += " "; }
		NewEditorSource = NewEditorSource.Replace(TEXT("\t"), *TabSpace);
		FString NewShaderSource = NewEditorSource.Replace(TEXT("\r\n"), TEXT("\n"));

		if (NewEditorSource == CurrentEditorSource && NewShaderSource == CurrentShaderSource)
		{
			return;
		}

		CurrentEditorSource = NewEditorSource;
		CurrentShaderSource = NewShaderSource;
		if (ShaderMultiLineEditableText)
		{
			SetText(FText::FromString(CurrentEditorSource));
		}
		CurEditState = !ShaderAssetObj->IsCompilable() || ShaderAssetObj->IsCompilationSuccessful() ? EditState::Succeed : EditState::Failed;
		SyncStageSelectorOptions(ShaderAssetObj->GetEnabledStageList());
	}

	void SShaderEditorBox::InitializeScrollBars()
	{
		SAssignNew(ShaderMultiLineVScrollBar, SMarkerScrollBar).Orientation(EOrientation::Orient_Vertical).Padding(0)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar")).Thickness(8.0f);
		ShaderMultiLineVScrollBar->OnSetState = [this](float InOffsetFraction, float InThumbSizeFraction) {
			LineNumberList->SetScrollOffset(InOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
			LineTipList->SetScrollOffset(InOffsetFraction * LineTipList->GetNumItemsBeingObserved());
		};
		SAssignNew(ShaderMultiLineHScrollBar, SScrollBar).Orientation(EOrientation::Orient_Horizontal).Padding(0)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FScrollBarStyle>("CustomScrollbar")).Thickness(8.0f);
	}

	void SShaderEditorBox::InitializeMarshalling()
	{
		ShaderMarshaller = MakeShared<FShaderEditorMarshaller>(this, MakeShared<ShaderTokenizer>());
		EffectMarshller = MakeShared<FShaderEditorEffectMarshaller>(this);
	}

	void SShaderEditorBox::StartLangServiceThreads()
	{
		//A single language-service thread builds one TU from CurrentShaderSource (unfolded)
		//and uses it for both code completion/diagnostics and syntax highlighting.
		LangEvent = FPlatformProcess::GetSynchEventFromPool();
		LangThread = MakeUnique<FThread>(TEXT("LangThread"), [this] {
			while (!bQuitLang)
			{
				LangTask Task;
				while (LangQueue.Dequeue(Task));
				if (Task.ShaderDesc)
				{
					TRefCountPtr<GpuShader> Shader = GGpuRhi->CreateShaderFromSource(Task.ShaderDesc->SourceDesc);
					Shader->CompileExtraArgs = Task.ShaderDesc->ExtraArgs;
					auto TU = ShaderTU::Create(Shader);
					DiagnosticInfos = TU->GetDiagnostic();

					CandidateInfos.Reset();
					if (!Task.CursorToken.IsEmpty())
					{
						CandidateInfos = TU->GetCodeComplete(Task.Row, Task.Col);
						if (Task.IsMemberAccess == false)
						{
							for (const auto& Candidate : DefaultCandidates(Shader->GetShaderLanguage(), Shader->GetShaderType()))
							{
								CandidateInfos.AddUnique(Candidate);
							}
						}

						CandidateInfos.RemoveAll([](const ShaderCandidateInfo& Candidate) {
							return Candidate.Text.StartsWith(TEXT("GPrivate_"));
						});

						for (auto It = CandidateInfos.CreateIterator(); It; ++It)
						{
							if (Task.IsMemberAccess)
							{
								if ((*It).Text.Find("operator") != INDEX_NONE)
								{
									It.RemoveCurrent();
									continue;
								}
								else if ((*It).Kind == ShaderCandidateKind::Type)
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

					LineSyntaxHighlightMaps.Reset();
					LineSyntaxHighlightMaps.SetNum(Task.LineTokens.Num());
					int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
					for (int LineIndex = 0; LineIndex < Task.LineTokens.Num(); LineIndex++)
					{
						//The TU is parsed from the unfolded source, so use the per-editor-line unfolded
						//line number captured at submission time to query the correct token type.
						int32 UnfoldedLineNumber = Task.LineNumbers[LineIndex];
						for (const ShaderTokenizer::Token& Token : Task.LineTokens[LineIndex])
						{
							ShaderTokenType NewTokenType = TU->GetTokenType(Token.Type, ExtraLineNum + UnfoldedLineNumber, Token.BeginOffset + 1, Token.EndOffset - Token.BeginOffset);
							LineSyntaxHighlightMaps[LineIndex].Add(FTextRange{ Token.BeginOffset, Token.EndOffset }, NewTokenType);
						}
					}
					this->SyntaxTU = MakeShareable(TU.Release());

					if (LangQueue.IsEmpty())
					{
						bRefreshLang.store(true, std::memory_order_release);
					}
				}

				if (LangQueue.IsEmpty())
				{
					LangEvent->Wait();
				}
			}
		});
	}

	void SShaderEditorBox::BuildEditorWidget(const FText& InitialShaderText)
	{
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
							.TextStyle(&FShaderHelperStyle::Get().GetWidgetStyle<FTextBlockStyle>("CodeEditorDefaultText"))
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
									MenuBuilder.AddMenuEntry(CodeEditorCommands::Get().GoToDefinition);
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
							.IsSelfDisabled(true)
							.Visibility(EVisibility::SelfHitTestInvisible)
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
						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						.HAlign(HAlign_Right)
						[
							SAssignNew(CodeSearchWidget, SCodeSearchWidget)
							.Visibility(EVisibility::Collapsed)
							.UICommandList(UICommandList)
							.OnTextChanged_Lambda([this](const FText& InTextToSearch) {
								ShaderMultiLineEditableText->GoTo(ShaderMultiLineEditableText->GetSelection().GetBeginning());
								ShaderMultiLineEditableText->SetSearchText(InTextToSearch);
								ScrollTo(ShaderMultiLineEditableText->GetSelection().GetBeginning().GetLineIndex());
								RefreshScrollBarMarkers();
							})
							.OnReplaced_Lambda([this](const FText& InTextToReplace, bool ReplaceAll) {
								FText CurSearchText = CodeSearchWidget->GetSearchText();
								if (!ReplaceAll)
								{
									int32 CurSearchIndex = ShaderMultiLineEditableTextLayout->CurrentSearchResultIndex;
									if (const FTextLocation* Loc = ShaderMultiLineEditableTextLayout->SearchResultToIndexMap.FindKey(CurSearchIndex))
									{
										FTextLocation StartLoc = *Loc;
										FTextLocation EndLoc = FTextLocation(StartLoc, CurSearchText.ToString().Len());
										ShaderMultiLineEditableText->SelectText(StartLoc, EndLoc);
										ShaderMultiLineEditableText->InsertTextAtCursor(InTextToReplace.ToString());
										ShaderMultiLineEditableText->Refresh();
										ShaderMultiLineEditableText->AdvanceSearch();
										ScrollTo(ShaderMultiLineEditableText->GetSelection().GetBeginning().GetLineIndex());
									}
								}
								else
								{
									SMultiLineEditableText::FScopedEditableTextTransaction TextTransaction(ShaderMultiLineEditableText);
									while (ShaderMultiLineEditableText->GetNumSearchResults() > 0)
									{
										const FTextLocation* Loc = ShaderMultiLineEditableTextLayout->SearchResultToIndexMap.FindKey(1);
										FTextLocation StartLoc = *Loc;
										FTextLocation EndLoc = FTextLocation(StartLoc, CurSearchText.ToString().Len());
										ShaderMultiLineEditableText->SelectText(StartLoc, EndLoc);
										ShaderMultiLineEditableText->InsertTextAtCursor(InTextToReplace.ToString());
									}
								}
								RefreshScrollBarMarkers();
							})
							.OnClosed_Lambda([this] {
								ShaderMultiLineEditableText->SetSearchText({});
								FSlateApplication::Get().SetUserFocus(0, ShaderMultiLineEditableText);
								RefreshScrollBarMarkers();
							})
							.OnMatchCaseChanged_Lambda([this](bool MatchCase) {
								ShaderMultiLineEditableTextLayout->SearchCase = MatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
								ShaderMultiLineEditableText->GoTo(ShaderMultiLineEditableText->GetSelection().GetBeginning());
								ShaderMultiLineEditableText->SetSearchText(CodeSearchWidget->GetSearchText());
								ScrollTo(ShaderMultiLineEditableText->GetSelection().GetBeginning().GetLineIndex());
								RefreshScrollBarMarkers();
							})
							.OnMatchWholeChanged_Lambda([this](bool MatchWhole) {
								ShaderMultiLineEditableTextLayout->MatchWhole = MatchWhole;
								ShaderMultiLineEditableText->GoTo(ShaderMultiLineEditableText->GetSelection().GetBeginning());
								ShaderMultiLineEditableText->SetSearchText(CodeSearchWidget->GetSearchText());
								ScrollTo(ShaderMultiLineEditableText->GetSelection().GetBeginning().GetLineIndex());
								RefreshScrollBarMarkers();
							})
							.SearchResultData_Lambda([this]() {
								SSearchBox::FSearchResultData Result{};
								Result.CurrentSearchResultIndex = ShaderMultiLineEditableText->GetSearchResultIndex();
								Result.NumSearchResults = ShaderMultiLineEditableText->GetNumSearchResults();

								return Result;
							})
							.OnResultNavigationButtonClicked_Lambda([this](SSearchBox::SearchDirection InDirection) {
								ShaderMultiLineEditableText->AdvanceSearch(InDirection == SSearchBox::SearchDirection::Previous);
								ScrollTo(ShaderMultiLineEditableText->GetSelection().GetBeginning().GetLineIndex());
							})
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
	}

	void SShaderEditorBox::ConfigureEditableTextLayout()
	{
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
			return TryMergeInsertUndoState(InUndoState);
		};
	}

	bool SShaderEditorBox::TryMergeInsertUndoState(SlateEditableTextTypes::FUndoState* InUndoState)
	{
		if(ShaderMultiLineEditableTextLayout->UndoStates.IsEmpty() || !bTryMergeUndoState)
		{
			return true;
		}

		SlateEditableTextTypes::FUndoState* LastState = ShaderMultiLineEditableTextLayout->UndoStates.Last().Get();
		if(LastState->Commands.Num() != 1 || InUndoState->Commands.Num() != 1)
		{
			return true;
		}

		TSharedPtr<SlateEditableTextTypes::FEditCommand> LastCmd = LastState->Commands[0];
		TSharedPtr<SlateEditableTextTypes::FEditCommand> CurCmd = InUndoState->Commands[0];
		if(LastCmd->GetType() != SlateEditableTextTypes::FEditCommand::Type::InsertAt
		   || CurCmd->GetType() != SlateEditableTextTypes::FEditCommand::Type::InsertAt)
		{
			return true;
		}

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

		return true;
	}

	void SShaderEditorBox::BindCommands()
	{
		UICommandList->MapAction(
			CodeEditorCommands::Get().Save,
			FExecuteAction::CreateLambda([this] {
				ShaderAssetObj->Save();
				Compile();
			})
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
		UICommandList->MapAction(CodeEditorCommands::Get().SelectAll,
			FExecuteAction::CreateLambda([this] {ShaderMultiLineEditableTextLayout->SelectAllText(); }),
			FCanExecuteAction::CreateLambda([this] { return ShaderMultiLineEditableTextLayout->CanExecuteSelectAll(); })
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().ToggleComment,
			FExecuteAction::CreateRaw(this, &SShaderEditorBox::ToggleComment)
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().GoToDefinition,
			FExecuteAction::CreateLambda([this] {
				GoToDefinitionAt(ShaderMultiLineEditableText->GetCursorLocation());
			})
		);
		UICommandList->MapAction(CodeEditorCommands::Get().Search,
			FExecuteAction::CreateLambda([this] {
				CodeSearchWidget->SetVisibility(EVisibility::Visible);
				FText SearchText;
				if (!ShaderMultiLineEditableText->AnyTextSelected())
				{
					FTextSelection WordSelection = GetWord(ShaderMultiLineEditableText->GetCursorLocation());
					ShaderMultiLineEditableText->SelectText(WordSelection.GetBeginning(), WordSelection.GetEnd());
				}
				SearchText = ShaderMultiLineEditableText->GetSelectedText();
				CodeSearchWidget->TriggerSearch(SearchText, false);
			})
		);
		UICommandList->MapAction(CodeEditorCommands::Get().Replace,
			FExecuteAction::CreateLambda([this] {
				FText SearchText;
				CodeSearchWidget->SetVisibility(EVisibility::Visible);
				if (!ShaderMultiLineEditableText->AnyTextSelected())
				{
					FTextSelection WordSelection = GetWord(ShaderMultiLineEditableText->GetCursorLocation());
					ShaderMultiLineEditableText->SelectText(WordSelection.GetBeginning(), WordSelection.GetEnd());
				}
				SearchText = ShaderMultiLineEditableText->GetSelectedText();
				CodeSearchWidget->TriggerSearch(SearchText, true);
			})
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
			FExecuteAction::CreateLambda([this] { MoveSelectedLines(-1); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().MoveLineDown,
			FExecuteAction::CreateLambda([this] { MoveSelectedLines(1); }),
			EUIActionRepeatMode::RepeatEnabled
		);
	}

	void SShaderEditorBox::MoveSelectedLines(int32 Direction)
	{
		const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
		const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		auto& Lines = ShaderMarshaller->TextLayout->GetLineModels();
		if ((Direction < 0 && StartLineIndex <= 0) || (Direction > 0 && EndLineIndex >= Lines.Num() - 1))
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
			int32 InsertLineIndex = Direction < 0 ? StartLineIndex - 1 : StartLineIndex + 1;
			Lines.Insert(MovedLines[Index], InsertLineIndex);
			//TextLayout does not provide an API to insert a single line.
			//To perform the insertion while preserving edit commands for an undo state, call OnAddLine() manually here.
			ShaderMarshaller->TextLayout->OnAddLine(InsertLineIndex, *MovedLines[Index].Text, BeforeOffsetLocations);
		}

		TArray<int32> AffectedBreakPoints;
		for (auto It = BreakPointLineNumbers.CreateIterator(); It; ++It)
		{
			int32 BreakPointLineIndex = GetLineIndex(*It);
			if (Direction < 0 && BreakPointLineIndex == StartLineIndex - 1)
			{
				AffectedBreakPoints.Add(GetLineNumber(EndLineIndex));
				It.RemoveCurrent();
			}
			else if (Direction > 0 && BreakPointLineIndex == EndLineIndex + 1)
			{
				AffectedBreakPoints.Add(GetLineNumber(StartLineIndex));
				It.RemoveCurrent();
			}
			else if (BreakPointLineIndex >= StartLineIndex && BreakPointLineIndex <= EndLineIndex)
			{
				AffectedBreakPoints.Add(GetLineNumber(BreakPointLineIndex + Direction));
				It.RemoveCurrent();
			}
		}
		BreakPointLineNumbers.Append(AffectedBreakPoints);

		const FTextLocation NewSelectionStart(StartLineIndex + Direction, Selection.GetBeginning().GetOffset());
		const FTextLocation NewSelectionEnd(EndLineIndex + Direction, Selection.GetEnd().GetOffset());
		ShaderMultiLineEditableText->SelectText(NewSelectionStart, NewSelectionEnd);
	}

	void SShaderEditorBox::InitializeCursorHighlighter()
	{
		TUniquePtr<FSlateEditableTextLayout>& ShaderEditableTextLayout = GetPrivate_SMultiLineEditableText_EditableTextLayout(*ShaderMultiLineEditableText);
		TSharedPtr<SlateEditableTextTypes::FCursorLineHighlighter>& CursorLineHighlighter = GetPrivate_FSlateEditableTextLayout_CursorLineHighlighter(*ShaderEditableTextLayout);
		SlateEditableTextTypes::FCursorInfo& CursorInfo = GetPrivate_FSlateEditableTextLayout_CursorInfo(*ShaderEditableTextLayout);

		CustomCursorHighlighter = CursorHighlighter::Create(&CursorInfo);
		CursorLineHighlighter = CustomCursorHighlighter;
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
		return GGpuRhi->CreateShaderFromSource(ShaderAssetObj->GetShaderDesc(CurrentShaderSource, SelectedStage).SourceDesc);;
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
		if (!ShaderAssetObj->IsCompilable())
		{
			return FStyleColors::Panel;
		}

        switch(CurEditState)
        {
		case EditState::Compiling: return FLinearColor{1.0f, 0.5f, 0.0f, 0.5f};
		case EditState::Failed:    return FLinearColor{1.0f, 0.1f, 0.1f, 0.5f};
		case EditState::Editing:   return FLinearColor{0.6f, 0.6f, 0.6f, 0.5f};
		default:                   return FLinearColor{0.6f, 1.0f, 0.1f, 0.45f};
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

	void SShaderEditorBox::SyncStageSelectorOptions(const TArray<FW::ShaderType>& InEnabledStages)
	{
		StageComboOptions.Empty();
		for (FW::ShaderType Type : InEnabledStages)
		{
			StageComboOptions.Add(MakeShared<FW::ShaderType>(Type));
		}

		if (InEnabledStages.Num() > 0 && !InEnabledStages.Contains(SelectedStage))
		{
			SelectedStage = InEnabledStages[0];
		}
		SubmitLangServiceTasks();
	}

    TSharedRef<SWidget> SShaderEditorBox::BuildInfoBar()
    {
        SAssignNew(InfoBarBox, SHorizontalBox);

		InfoBarBox->ClearChildren();

		FSlateFontInfo InforBarFontInfo = FShaderHelperStyle::Get().GetFontStyle("CodeFont");
		if (ShaderAssetObj->IsCompilable())
		{
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
					.Text(this, &SShaderEditorBox::GetEditStateText)
				]
			];
		}

		InfoBarBox->AddSlot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			];

		// Stage selector for shader assets that expose enabled stages
		TArray<FW::ShaderType> EnabledStages = ShaderAssetObj->GetEnabledStageList();
		if (EnabledStages.Num() > 0)
		{
			StageComboOptions.Empty();
			for (FW::ShaderType Type : EnabledStages)
			{
				StageComboOptions.Add(MakeShared<FW::ShaderType>(Type));
			}

			TSharedPtr<FW::ShaderType> InitialSelection;
			for (auto& Option : StageComboOptions)
			{
				if (*Option == SelectedStage)
				{
					InitialSelection = Option;
					break;
				}
			}
			if (!InitialSelection && StageComboOptions.Num() > 0)
			{
				SelectedStage = *StageComboOptions[0];
				InitialSelection = StageComboOptions[0];
			}

			InfoBarBox->AddSlot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SComboBox<TSharedPtr<FW::ShaderType>>)
				.ToolTipText(LOCALIZATION("StageSelectorTip"))
				.OptionsSource(&StageComboOptions)
				.InitiallySelectedItem(InitialSelection)
				.OnGenerateWidget_Lambda([](TSharedPtr<FW::ShaderType> InItem) {
					return SNew(STextBlock).Text(FText::FromString(ANSI_TO_TCHAR(magic_enum::enum_name(*InItem).data())));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FW::ShaderType> InSelection, ESelectInfo::Type) {
					if (InSelection.IsValid())
					{
						SelectedStage = *InSelection;
						SubmitLangServiceTasks();
					}
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this] {
						return FText::FromString(ANSI_TO_TCHAR(magic_enum::enum_name(SelectedStage).data()));
					})
				]
			];
		}

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

	void SShaderEditorBox::GoToDefinitionAt(const FTextLocation& Location)
	{
		if (!ShaderMarshaller->TokenizedLines.IsValidIndex(Location.GetLineIndex()))
		{
			return;
		}
		const auto& TokenizedLine = ShaderMarshaller->TokenizedLines[Location.GetLineIndex()];
		
		// Check if this is an #include line
		FString IncludePath;
		FString LineText;
		ShaderMultiLineEditableText->GetTextLine(Location.GetLineIndex(), LineText);
		std::regex IncludePattern(R"(#include\s*["<]([^"<>]+)[">])");
		std::smatch Match;
		std::string LineTextStd(TCHAR_TO_UTF8(*LineText));
		if (std::regex_search(LineTextStd, Match, IncludePattern) && Match.size() > 1)
		{
			IncludePath = UTF8_TO_TCHAR(Match[1].str().c_str());
		}
		
		if (!IncludePath.IsEmpty())
		{
			AssetPtr<ShaderAsset> TargetShaderAsset = ShaderAssetObj->FindIncludeAsset(IncludePath);
			if (TargetShaderAsset)
			{
				AssetOp::OpenAsset(TargetShaderAsset);
			}
			return;
		}
		
		int32 TokenOffset{};
		int32 TokenEndOffset{};
		for (const auto& Token : TokenizedLine.Tokens)
		{
			if (Token.Type == ShaderTokenType::Identifier && Location.GetOffset() >= Token.BeginOffset && Location.GetOffset() <= Token.EndOffset)
			{
				TokenOffset = Token.BeginOffset;
				TokenEndOffset = Token.EndOffset;
				break;
			}
		}
		if (SyntaxTUCopy.IsValid())
		{
			int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
			//The TU is built from the unfolded source, so map the editor line back to its unfolded line number.
			int32 UnfoldedLineNumber = GetLineNumber(Location.GetLineIndex());
			auto Symbol = SyntaxTUCopy->GetSymbolInfo(UnfoldedLineNumber + ExtraLineNum, TokenOffset + 1, TokenEndOffset - TokenOffset);
			
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			int32 SymbolExtraLineNum = ExtraLineNum;
			AssetPtr<ShaderAsset> TargetShaderAsset = ShaderAssetObj->FindIncludeAsset(Symbol.File);
			
			if (TargetShaderAsset)
			{
				SymbolExtraLineNum = TargetShaderAsset->GetExtraLineNum();
			}
			
			int SymbolLineNumber = Symbol.Row - SymbolExtraLineNum;
			if (SymbolLineNumber > 0)
			{
				if (TargetShaderAsset)
				{
					AssetOp::OpenAsset(TargetShaderAsset);
					SShaderEditorBox* NewShaderEditor = ShEditor->GetShaderEditor(TargetShaderAsset);
					if (NewShaderEditor)
					{
						auto FrameCount = MakeShared<int32>(2);
						FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
							[NewShaderEditor, SymbolLineNumber, FrameCount](float) {
								if (--(*FrameCount) > 0) return true;
								NewShaderEditor->JumpTo(NewShaderEditor->GetLineIndex(SymbolLineNumber));
								return false;
							}));
					}
				}
				else if (Symbol.File.IsEmpty() || Symbol.File == ShaderAssetObj->GetShaderName())
				{
					// Jump to definition in current file
					JumpTo(GetLineIndex(SymbolLineNumber));
				}
			}
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
				if (StartLineNumber == *It && EndLineNumber == *It)
					continue;

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

        if(DeltaLineCount != 0)
        {
            if(LineNumberList) {
                LineNumberList->RequestListRefresh();
            }
            
            if(LineTipList){
                LineTipList->RequestListRefresh();
            }
        }
    }

	void SShaderEditorBox::ClearDiagInfoEffect()
	{
		EffectMarshller->LineNumberToDiagInfo.Reset();
		EffectMarshller->MakeDirty();
		EffectMultiLineEditableText->Refresh();
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

	void SShaderEditorBox::SubmitLangServiceTasks()
	{
		LangTask Task{};
		Task.ShaderDesc = ShaderAssetObj->GetShaderDesc(CurrentShaderSource, SelectedStage);

		if (bTryComplete)
		{
			const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
			const int32 CursorRow = CursorLocation.GetLineIndex();
			const int32 CursorCol = CursorLocation.GetOffset();
			int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();

			FString CurLineText;
			ShaderMultiLineEditableText->GetTextLine(CursorRow, CurLineText);
			FString CursorLeft = CurLineText.Mid(0, CursorCol);

			TArray<ShaderTokenizer::TokenizedLine> TokenizedLines = ShaderMarshaller->Tokenizer->Tokenize(CursorLeft, true);
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

		auto& LineModels = ShaderMarshaller->TextLayout->GetLineModels();
		Task.LineTokens.SetNum(LineModels.Num());
		Task.LineNumbers.SetNum(LineModels.Num());
		for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
		{
			//Snapshot the unfolded line number so the worker thread can map editor lines back
			//to lines in CurrentShaderSource even if the main thread modifies LineNumberData later.
			int32 UnfoldedLineNumber = GetLineNumber(LineIndex);
			Task.LineNumbers[LineIndex] = UnfoldedLineNumber;

			ShaderTokenizer::TokenizedLine* CurTokenizedLine = static_cast<ShaderTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
			if (CurTokenizedLine)
			{
				for (const ShaderTokenizer::Token& Token : CurTokenizedLine->Tokens)
				{
					Task.LineTokens[LineIndex].Add(Token);
				}
			}
		}
		bRefreshLang.store(false, std::memory_order_relaxed);
		LangQueue.Enqueue(MoveTemp(Task));
		LangEvent->Trigger();
	}

	void SShaderEditorBox::RefreshSyntaxHighlight()
	{
		TArray<Vector2u> InactiveLineRange = SyntaxTUCopy->GetInactiveRegions();
		int32 ExtraLineNum = ShaderAssetObj->GetExtraLineNum();
		
		// Returns the index of the inactive region the line is in, or INDEX_NONE if not inactive.
		// The TU is built from the unfolded source, so map the editor line back to its
		// unfolded line number via GetLineNumber to compensate for code folding.
		auto GetInactiveRegionIndex = [this, &InactiveLineRange, ExtraLineNum](int32 LineIndex) -> int32 {
			int32 UnfoldedLineNumber = GetLineNumber(LineIndex);
			int32 LineNumber = UnfoldedLineNumber + ExtraLineNum;
			for (int32 i = 0; i < InactiveLineRange.Num(); i++)
			{
				const Vector2u& Range = InactiveLineRange[i];
				if (LineNumber >= (int32)Range.x && LineNumber <= (int32)Range.y)
				{
					return i;
				}
			}
			return INDEX_NONE;
		};
		
		for(int LineIndex = 0; LineIndex < LineSyntaxHighlightMapsCopy.Num(); LineIndex++)
		{
			auto& SyntaxHighlightMap = LineSyntaxHighlightMapsCopy[LineIndex];
			auto& LineModel =  ShaderMarshaller->TextLayout->GetLineModels()[LineIndex];
			
			bool bIsInactive = GetInactiveRegionIndex(LineIndex) != INDEX_NONE;
			
			for (const auto& [TokenRange, TokenType] : SyntaxHighlightMap)
			{
				for (auto& RunModel : LineModel.Runs)
				{
					TSharedRef<IRun> Run = RunModel.GetRun();
					if (Run->GetTextRange() == TokenRange)
					{
						RunModel = FTextLayout::FRunModel(FSlateTextStyleRefRun::Create(FRunInfo(), LineModel.Text, bIsInactive ? GetDimTokenStyleMap()[TokenType] : GetTokenStyleMap()[TokenType], TokenRange));
					}
				}
			}

		}
		
		//Build BracketGroups and FoldingBraceGroups, skipping brackets where one side is in a different inactive region than the other
		{
			auto& LineModels = ShaderMarshaller->TextLayout->GetLineModels();
			
			struct BracketStackData
			{
				int32 LineIndex{};
				int32 InactiveRegionIndex{};  // INDEX_NONE if not in inactive region
				ShaderTokenizer::Bracket Bracket;
			};
			TArray<ShaderTokenizer::BracketGroup> BraceGroups, ParenGroups;
			TArray<BracketStackData> OpenBraceStack, OpenParenStack;
			
			for (int32 LineIndex = 0; LineIndex < LineModels.Num(); LineIndex++)
			{
				ShaderTokenizer::TokenizedLine* TokenizedLine = static_cast<ShaderTokenizer::TokenizedLine*>(LineModels[LineIndex].CustomData.Get());
				if (!TokenizedLine)
				{
					continue;
				}
				
				int32 CurrentInactiveRegionIndex = GetInactiveRegionIndex(LineIndex);
				
				for (const ShaderTokenizer::Bracket& Brace : TokenizedLine->Braces)
				{
					if (Brace.Type == ShaderTokenizer::SideType::Open)
					{
						OpenBraceStack.Emplace(LineIndex, CurrentInactiveRegionIndex, Brace);
					}
					else
					{
						if (!OpenBraceStack.IsEmpty())
						{
							auto OpenBraceData = OpenBraceStack.Pop();
							// Skip if not in the same inactive region (both must be in same region or both not inactive)
							if (OpenBraceData.InactiveRegionIndex != CurrentInactiveRegionIndex)
							{
								continue;
							}
							BraceGroups.Emplace(OpenBraceData.LineIndex, OpenBraceData.Bracket, LineIndex, Brace);
						}
					}
				}
				
				for (const ShaderTokenizer::Bracket& Paren : TokenizedLine->Parens)
				{
					if (Paren.Type == ShaderTokenizer::SideType::Open)
					{
						OpenParenStack.Emplace(LineIndex, CurrentInactiveRegionIndex, Paren);
					}
					else
					{
						if (!OpenParenStack.IsEmpty())
						{
							auto OpenParenData = OpenParenStack.Pop();
							// Skip if not in the same inactive region (both must be in same region or both not inactive)
							if (OpenParenData.InactiveRegionIndex != CurrentInactiveRegionIndex)
							{
								continue;
							}
							ParenGroups.Emplace(OpenParenData.LineIndex, OpenParenData.Bracket, LineIndex, Paren);
						}
					}
				}
			}
			
			ShaderMarshaller->BracketGroups.Empty();
			ShaderMarshaller->BracketGroups.Append(BraceGroups);
			ShaderMarshaller->BracketGroups.Append(ParenGroups);
			
			ShaderMarshaller->FoldingBraceGroups.Empty();
			for (const auto& BraceGroup : BraceGroups)
			{
				if (BraceGroup.OpenLineIndex != BraceGroup.CloseLineIndex && !ShaderMarshaller->FoldingBraceGroups.Contains(BraceGroup.OpenLineIndex))
				{
					ShaderMarshaller->FoldingBraceGroups.Add(BraceGroup.OpenLineIndex, BraceGroup);
				}
			}
			RefreshBracketHighlight();
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
			for (const ShaderTokenizer::Token& Token : ShaderMarshaller->TokenizedLines[CursorLineIndex].Tokens)
			{
				if (CursorOffset >= Token.BeginOffset && CursorOffset <= Token.EndOffset
					&& (Token.Type == ShaderTokenType::Identifier || Token.Type == ShaderTokenType::Preprocess)
					&& SyntaxTUCopy)
				{
					Occurrences = SyntaxTUCopy->GetOccurrences(GetLineNumber(CursorLineIndex) + AddedLineNum, Token.BeginOffset + 1);
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
					for (const ShaderTokenizer::Token& Token : ShaderMarshaller->TokenizedLines[OccurrenceLineIndex].Tokens)
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

	void SShaderEditorBox::RefreshScrollBarMarkers()
	{
		TArray<FScrollBarMarker> AllMarkers;

		for (const auto& [LineNumber, DiagInfo] : EffectMarshller->LineNumberToDiagInfo)
		{
			int32 LineIndex = GetLineIndex(LineNumber);
			if (LineIndex != INDEX_NONE)
			{
				FLinearColor Color = DiagInfo.IsError ? FLinearColor::Red : FLinearColor::Yellow;
				AllMarkers.Add({ LineIndex, Color });
			}
		}

		for (const FTextLineHighlight& Highlight : OccurrenceHighlights)
		{
			AllMarkers.Add({ Highlight.LineIndex, FLinearColor::Gray });
		}

		for (const auto& Pair : ShaderMultiLineEditableTextLayout->SearchResultToIndexMap)
		{
			AllMarkers.Add({ Pair.Key.GetLineIndex(), FLinearColor(0.8f, 0.6f, 0.1f, 0.85f) });
		}

		ShaderMultiLineVScrollBar->SetTotalLineCount(GetCurDisplayLineCount() + PaddingLineNum);
		ShaderMultiLineVScrollBar->SetMarkers(AllMarkers);
	}

	void SShaderEditorBox::RefreshLineTips()
	{
		if (LineTipList)
		{
			LineTipList->RebuildList();
		}
	}

    void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
		//ShaderMultiLineEditableText updates its VScrollBar when it ticks, and we sync the LineNumberList/LineTipList according to its VScrollBar callback.
		//Which leads to a delay of one frame in the drawing of the list, so here we tick it earlier.
		ShaderMultiLineEditableTextLayout->Tick(ShaderMultiLineEditableText->GetTickSpaceGeometry(), InCurrentTime, InDeltaTime);
        
		if (bRefreshLang.load(std::memory_order_acquire))
		{
			SyntaxTUCopy = SyntaxTU;
			LineSyntaxHighlightMapsCopy = LineSyntaxHighlightMaps;

			RefreshLineNumberToDiagInfo();
			RefreshScrollBarMarkers();
			RefreshCodeCompletionTip();
			RefreshSyntaxHighlight();
			bRefreshLang.store(false, std::memory_order_relaxed);
		}

        UpdateEffectText();
    }

	void SShaderEditorBox::OnCursorMoved(const FTextLocation& InLocation)
	{
		if(!bKeyChar) {
			bTryComplete = false;
			bTryMergeUndoState = false;
			RefreshOccurrenceHighlight();
			RefreshScrollBarMarkers();
		}
		bKeyChar = false;
		RefreshBracketHighlight();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->AddNavigationInfo(ShaderAssetObj, InLocation);

		TSharedPtr<SWindow> ShaderEditorTipWindow = ShEditor->GetShaderEditorTipWindow();
		ShaderEditorTipWindow->SetContent(SNullWidget::NullWidget);
		ShaderEditorTipWindow->HideWindow();
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
		if (InsertCol == INDEX_NONE)
		{
			return;
		}

		int32 ReplaceLen = CurToken.Len();
        if(CurToken == ".")
        {
            InsertCol += 1;
			ReplaceLen = 0;
        }
        
        bTryComplete = false;
        FSlateTextLayout* ShaderTextLayout = static_cast<FSlateTextLayout*>(ShaderMarshaller->TextLayout);
        const FTextLocation NewCursorLocation(CursorRow, InsertCol);
		
		//TextLayout->RemoveAt does not handle the cursor, so manually call the goto function.
		//However, directly call SelectText and DeleteSelectedText also can do it.
		ShaderTextLayout->RemoveAt({CursorRow, InsertCol}, ReplaceLen);
        ShaderMultiLineEditableText->GoTo(NewCursorLocation);
		
		FString InsertedText = InItem->Text;
		if(InItem->Kind == ShaderCandidateKind::Func)
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
		if (!CanRealTimeDiagnosis())
		{
			return;
		}

        EffectMarshller->LineNumberToDiagInfo.Reset();
        for (const ShaderDiagnosticInfo& DiagInfo : DiagnosticInfos)
        {
            bool bIsMainFile = DiagInfo.File == ShaderAssetObj->GetShaderName();
			if (!bIsMainFile)
			{
				continue;
			}

			int32 DiagInfoLineNumber = (int32)DiagInfo.Row - ShaderAssetObj->GetExtraLineNum();
            if (!EffectMarshller->LineNumberToDiagInfo.Contains(DiagInfoLineNumber))
            {
                int32 LineIndex = GetLineIndex(DiagInfoLineNumber);
                FString DummyText;
                if (LineIndex != INDEX_NONE) {
                    ShaderMultiLineEditableText->GetTextLine(LineIndex, DummyText);
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
        
		EffectMarshller->MakeDirty();
		EffectMultiLineEditableText->Refresh();
    }

	FTextSelection SShaderEditorBox::GetWord(const FTextLocation& InTextLocation) const
	{
		int32 LineIndex = InTextLocation.GetLineIndex();
		FString LineStr;
		ShaderMultiLineEditableText->GetTextLine(LineIndex, LineStr);
		int32 Offset = InTextLocation.GetOffset();

		if (LineStr.IsEmpty() || InTextLocation.GetOffset() >= LineStr.Len())
		{
			return { InTextLocation, InTextLocation };
		}

		if (!FChar::IsIdentifier(LineStr[Offset]))
		{
			return { InTextLocation, InTextLocation };
		}
    
		int32 WordStart = Offset;
		while (WordStart > 0 && FChar::IsIdentifier(LineStr[WordStart - 1]))
		{
			WordStart--;
		}
    
		int32 WordEnd = Offset;
		while (WordEnd < LineStr.Len() && FChar::IsIdentifier(LineStr[WordEnd]))
		{
			WordEnd++;
		}
   
		return { FTextLocation(LineIndex, WordStart), FTextLocation(LineIndex, WordEnd)};
	}

	FString AdjustDiagLineNumber(const FString& DiagInfo, const FString& MainFileName, int32 Delta)
	{
		std::string DiagString{ TCHAR_TO_UTF8(*DiagInfo) };
		std::regex Pattern{ "(.+?):([0-9]+)(?::([0-9]+))?:\\s*(error|warning):" };
		std::smatch Match;
		std::size_t SearchPos = 0;
		while (std::regex_search(DiagString.cbegin() + SearchPos, DiagString.cend(), Match, Pattern))
		{
			FString DiagFileName = UTF8_TO_TCHAR(Match[1].str().c_str());
			std::size_t NextSearchPos = SearchPos + Match.position() + Match.length();
			if (DiagFileName == MainFileName)
			{
				std::string RowStr = Match[2];
				std::string RowNumber = std::to_string(std::stoi(RowStr) + Delta);
				DiagString.replace(SearchPos + Match.position(2), Match[2].length(), RowNumber);
				NextSearchPos += RowNumber.length() - RowStr.length();
			}
			SearchPos = NextSearchPos;
		}
		return FString{ UTF8_TO_TCHAR(DiagString.data()) };
	}

	void SShaderEditorBox::Compile()
    {
		if (!ShaderAssetObj->IsCompilable())
		{
			CurEditState = EditState::Succeed;
			return;
		}

		int32 AddedLineNum = ShaderAssetObj->GetExtraLineNum();
		
		//TODO Async and show "Compiling" state
		FString ErrorInfo, WarnInfo;
        if (ShaderAssetObj->CompileShader(ErrorInfo, WarnInfo))
        {
			ShaderAssetObj->OnShaderRefreshed.Broadcast();
            CurEditState = EditState::Succeed;
			
			if(!WarnInfo.IsEmpty())
			{
				WarnInfo = AdjustDiagLineNumber(WarnInfo, ShaderAssetObj->GetShaderName(), -AddedLineNum);
				SH_LOG(LogShader, Warning, TEXT("%s"), *WarnInfo);
			}
        }
		else
		{
			CurEditState = EditState::Failed;
			
			if(!ErrorInfo.IsEmpty())
			{
				ErrorInfo = AdjustDiagLineNumber(ErrorInfo, ShaderAssetObj->GetShaderName(), -AddedLineNum);
				SH_LOG(LogShader, Error, TEXT("%s"), *ErrorInfo);
			}
		}
    }

    void SShaderEditorBox::OnShaderTextChanged(const FString& InShaderSouce)
    {
        FString NewShaderSource = InShaderSouce.Replace(TEXT("\r\n"), TEXT("\n"));
        if (NewShaderSource != ShaderAssetObj->EditorContent)
        {
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			if(ShEditor->GetDebugger().IsValid())
			{
				ShEditor->GetDebugger().MarkEditDuringDebugging();
			}
			CurEditState = EditState::Editing;
			ShaderAssetObj->EditorContent = NewShaderSource;
			ShaderAssetObj->MarkDirty(NewShaderSource != ShaderAssetObj->SavedEditorContent);
        }
        
		CurrentEditorSource = NewShaderSource;
        CurrentShaderSource = NewShaderSource;

		SubmitLangServiceTasks();
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
		bShortcutHandled = false;
        const FKey Key = InKeyEvent.GetKey();
		
		const FTextSelection& Selection = ShaderMultiLineEditableText->GetSelection();
		const int32 StartLineIndex = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineIndex = Selection.GetEnd().GetLineIndex();
		
		const FTextLocation& CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
		const int32 CursorLine = CursorLocation.GetLineIndex();
		const int32 CursorOffset = CursorLocation.GetOffset();

		if (UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			bShortcutHandled = true;
			return FReply::Handled();
		}
		
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->GetUICommandList()->ProcessCommandBindings(InKeyEvent))
		{
			bShortcutHandled = true;
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
        }
		else
		{
			if(Key == EKeys::Enter)
			{
				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(ShaderMultiLineEditableText);
				ShaderMultiLineEditableTextLayout->HandleCarriageReturn(InKeyEvent.IsRepeat());
				// at this point, the text after the text cursor is already in a new line
				HandleAutoIndent();
			}
		}

		if (Key == EKeys::Left)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				FIntPoint(-1, 0),
				ECursorAction::MoveCursor
			));
		}
		else if (Key == EKeys::Right)
		{
			ShaderMultiLineEditableTextLayout->MoveCursor(FMoveCursor::Cardinal(
				ECursorMoveGranularity::Character,
				FIntPoint(+1, 0),
				ECursorAction::MoveCursor
			));
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
		}

		return FReply::Handled();
    }

	void SShaderEditorBox::SetText(const FText& Text)
	{
		ShaderMultiLineEditableText->SelectAllText();
		ShaderMultiLineEditableText->InsertTextAtCursor(Text);
	}

	FReply SShaderEditorBox::HandleKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
    {
        TSharedPtr<SMultiLineEditableText> Text = ShaderMultiLineEditableText;
		if (bShortcutHandled)
		{
			return FReply::Handled();
		}
            
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
		auto GetDebugger = []() -> ShaderDebugger& {
			return static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebugger();
		};
		
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
                if(LineIndex >= BeginLineIndex && LineIndex <= EndLineIndex)
                {
                    return FStyleColors::Foreground.GetSpecifiedColor();
                }
                return FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity(0.5f);
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
                    return FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity(FoldingArrowAnim.GetLerp());
                }
                return FStyleColors::Foreground.GetSpecifiedColor();
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
			else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
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
			else if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
			{
				return FStyleColors::Error;
			}
			else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
			{
				return FLinearColor::Green;
			}
			return FStyleColors::Error;
		})
		.Visibility_Lambda([=, this] {
			if(IsErrorLine(LineNumber) || IsWarningLine(LineNumber))
			{
				return EVisibility::Visible;
			}
			else if(BreakPointLineNumbers.Contains(LineNumber) || (GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber))
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
					.Visibility_Lambda([this, LineNumber, GetDebugger]{
						if(BreakPointLineNumbers.Contains(LineNumber) || (GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber, GetDebugger]{
							if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
							{
								return FLinearColor{ 1,0,0,0.7f };
							}
							else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
							{
								return FLinearColor{0,1,0,0.7f};
							}
							return FLinearColor{ 1,0,0,0.7f };
						})
					]
				]
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Visibility_Lambda([this, LineNumber, GetDebugger]{
						if(BreakPointLineNumbers.Contains(LineNumber) || GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.Image(FAppStyle::Get().GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda([this, LineNumber, GetDebugger]{
						if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
						{
							return FLinearColor{ 1,0,0,0.06f };
						}
						else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
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
		auto GetDebugger = []() -> ShaderDebugger& {
			return static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetDebugger();
		};
		
		float LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
		auto LineHeightBox = SNew(SBox).HeightOverride(LineHeight).Visibility(EVisibility::HitTestInvisible);
		auto LineTip = SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineTipItemStyle"))
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Content()
			[
				LineHeightBox
			];

		//Dummy
		if (Item->ToString() == "-")
		{
			return SNew(STableRow<LineNumberItemPtr>, OwnerTable).Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
				.Visibility(EVisibility::SelfHitTestInvisible)
				.Content()[LineHeightBox];
		}

		int32 LineNumber = FCString::Atoi(*(*Item).ToString());

		//Create preview viewport for this line if a preview texture exists
		TSharedPtr<FW::PreviewViewPort> LinePreviewViewPort;
		DebuggerLocation LineLoc;
		{
			ShaderDebugger& Debugger = GetDebugger();
			ShaderAsset* DebuggerShaderAsset = Debugger.GetShaderAsset();
			const auto& PreviewTextures = Debugger.GetLinePreviewTextures();
			if (DebuggerShaderAsset && !PreviewTextures.IsEmpty())
			{
				for (const auto& [PreviewLoc, Texture] : PreviewTextures)
				{
					if (PreviewLoc.LineNumber == LineNumber && DebuggerShaderAsset->FindIncludeAsset(PreviewLoc.File) == ShaderAssetObj)
					{
						LineLoc = PreviewLoc;
						LinePreviewViewPort = MakeShared<FW::PreviewViewPort>();
						LinePreviewViewPort->SetViewPortRenderTexture(Texture.GetReference());
						break;
					}
				}
			}
		}
		
		LineTip->SetContent(
				SNew(SOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+SOverlay::Slot()
				[
					LineHeightBox
				]
				+SOverlay::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(1.0)
					.Visibility_Lambda([this, LineNumber, GetDebugger]{
						if(BreakPointLineNumbers.Contains(LineNumber) || (GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect2"))
						.ColorAndOpacity_Lambda([this, LineNumber, GetDebugger]{
							if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
							{
								return FLinearColor{ 1,0,0,0.7f };
							}
							else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
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
					.Visibility_Lambda([this, LineNumber, GetDebugger]{
						if(BreakPointLineNumbers.Contains(LineNumber) || (GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber))
						{
							return EVisibility::HitTestInvisible;
						}
						return EVisibility::Collapsed;
					})
					.BorderImage_Lambda([this, LineNumber, GetDebugger]{
						if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
						{
							return FAppStyle::Get().GetBrush("WhiteBrush");
						}
						return FShaderHelperStyle::Get().GetBrush("LineTip.BreakPointEffect");
					})
					.BorderBackgroundColor_Lambda([this, LineNumber, GetDebugger]{
						if (!GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber)
						{
							return FLinearColor{ 1,0,0,0.3f };
						}
						else if(GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetStopLocation().File) == ShaderAssetObj && GetDebugger().GetStopLocation().LineNumber == LineNumber)
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
						.Visibility_Lambda([this, LineNumber, GetDebugger] { 
							return !GetDebugger().GetDebuggerError().Key.IsEmpty() && GetDebugger().GetShaderAsset()->FindIncludeAsset(GetDebugger().GetDebuggerError().Value.File) == ShaderAssetObj && GetDebugger().GetDebuggerError().Value.LineNumber == LineNumber ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
						})
						.Text_Lambda([GetDebugger] { 
							return FText::FromString(GetDebugger().GetDebuggerError().Key); 
						})
						.ColorAndOpacity(FLinearColor::Red)
					]
				]
				+SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.HeightOverride(LineHeight)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([LineLoc] {
							auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
							bool bActive = ShEditor->IsShowingLinePreview() && ShEditor->GetLinePreviewLocation() == LineLoc;
							return bActive ? FStyleColors::AccentOrange : FStyleColors::Transparent;
						})
						.Padding(1.0f)
						.Visibility_Lambda([LinePreviewViewPort, GetDebugger, LineLoc] {
							return LinePreviewViewPort.IsValid() && GetDebugger().IsValid() && !GetDebugger().GetLinePreviewData().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
						})
						.OnMouseButtonDown_Lambda([LineLoc](const FGeometry&, const FPointerEvent&) {
							auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
							if (ShEditor->IsShowingLinePreview() && ShEditor->GetLinePreviewLocation() == LineLoc)
							{
								ShEditor->DismissLinePreview();
							}
							else
							{
								ShEditor->ShowLinePreview(LineLoc);
							}
							return FReply::Handled();
						})
						[
							SNew(SViewport)
							.ViewportInterface(LinePreviewViewPort)
							.ViewportSize(FVector2D(LineHeight, LineHeight))
						]
					]
				]
        );

    
        LineTip->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([this, LineNumber, GetDebugger]{
            const FTextLocation CursorLocation = ShaderMultiLineEditableText->GetCursorLocation();
            const int32 CurLineIndex = CursorLocation.GetLineIndex();
            auto FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
            if(CanHighlightCursorLine() && FocusedWidget == ShaderMultiLineEditableText && LineNumber == GetLineNumber(CurLineIndex)
			   && !BreakPointLineNumbers.Contains(LineNumber) && GetDebugger().GetStopLocation().LineNumber != LineNumber)
            {
                double CurTime = FPlatformTime::Seconds();
                float Speed = 1.8f;
                double AnimatedOpacity = 0.08 * FMath::Pow(FMath::Abs(FMath::Sin(CurTime * Speed)),1.8) + 0.1;
				return FStyleColors::Foreground.GetSpecifiedColor().CopyWithNewOpacity((float)AnimatedOpacity);
            }
            return FLinearColor::Transparent;
        }));
        
        return LineTip;
    }

	void SShaderEditorBox::JumpTo(const FTextLocation& InLocation)
	{
		int32 LineCount = ShaderMarshaller->TextLayout->GetLineCount();
		int32 InLineIndex = InLocation.GetLineIndex();

		double LineHeight = ShaderMarshaller->TextLayout->GetUniformLineHeight() / ShaderMarshaller->TextLayout->GetScale();
		double ViewHeight = ShaderMultiLineEditableText->GetTickSpaceGeometry().GetLocalSize().Y;
		int32 VisibleLineCount = static_cast<int32>(ViewHeight / LineHeight);
		int32 StartVisibleLineIndex = ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
		int32 EndVisibleLineIndex = ShaderMarshaller->TextLayout->GetEndVisibleLineIndex();
		int32 Offset = VisibleLineCount / 5;

		if (InLineIndex <= StartVisibleLineIndex + Offset || InLineIndex >= EndVisibleLineIndex - Offset)
		{
			double TargetScrollY = LineHeight * FMath::Max(0, InLineIndex - VisibleLineCount / 2);

			FVector2D CurrentOffset = ShaderMultiLineEditableTextLayout->GetScrollOffset();
			ShaderMultiLineEditableTextLayout->SetScrollOffset(
				FVector2D(CurrentOffset.X, TargetScrollY),
				ShaderMultiLineEditableText->GetTickSpaceGeometry()
			);
		}
		ShaderMultiLineEditableText->GoTo(InLocation);

		// Move the cursor without scrolling
		GetPrivate_FSlateEditableTextLayout_PositionToScrollIntoView(*ShaderMultiLineEditableTextLayout).Reset();
	}

	void SShaderEditorBox::ScrollTo(const FTextLocation& InLocation)
	{
		int32 LineCount = ShaderMarshaller->TextLayout->GetLineCount();
		int32 VisibleLineCount = ShaderMarshaller->TextLayout->GetVisibleLineCount();
		int32 StartVisibleLineIndex = ShaderMarshaller->TextLayout->GetStartVisibleLineIndex();
		int32 EndVisibleLineIndex = ShaderMarshaller->TextLayout->GetEndVisibleLineIndex();
		int32 Offset = VisibleLineCount / 5;
		int32 InLineIndex = InLocation.GetLineIndex();
		if (InLineIndex <= StartVisibleLineIndex + Offset)
		{
			int32 TargetLineIndex = FMath::Clamp(InLineIndex - VisibleLineCount / 2, 0, LineCount - 1);
			ShaderMultiLineEditableText->ScrollTo({ TargetLineIndex, 0 });
		}
		else if (InLineIndex >= EndVisibleLineIndex - Offset)
		{
			int32 TargetLineIndex = FMath::Clamp(InLineIndex + VisibleLineCount / 2, 0, LineCount - 1);
			ShaderMultiLineEditableText->ScrollTo({ TargetLineIndex, 0 });
		}
	}
}
