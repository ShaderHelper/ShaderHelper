#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/Widgets/ShaderCodeEditor/ShaderCodeTokenizer.h"
#include "Renderer/ShRenderer.h"
#include "GpuApi/GpuShader.h"
#include <Internationalization/IBreakIterator.h>
#include <Widgets/SCanvas.h>
#include <Widgets/Text/SlateEditableTextLayout.h>
#include "AssetObject/ShaderAsset.h"
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/Spirv/SpirvPixelVM.h"
#include "GpuApi/GpuBindGroup.h"

namespace SH
{
    class SShaderEditorBox;

    struct FoldMarker
    {
        //If the marker is visible top level, then the index is relative to 0;
        //otherwise, relative to the parent index
        int32 RelativeLineIndex;
        int32 Offset;
        TArray<FString> FoldedLineTexts;
        
        //Invisible
        TArray<FoldMarker> ChildFoldMarkers;

        FString GetTotalFoldedLineTexts() const;
        int32 GetFoldedLineCounts() const;
    };

    struct ShaderUndoState : SlateEditableTextTypes::FUndoState
    {
        TArray<FoldMarker> FoldMarkers;
		TArray<int32> BreakPointLines;
    };

	class FShaderEditorMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<HlslTokenizer> InTokenizer);
		
	public:
		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

		//ReTokenize when the text changed.
		bool RequiresLiveUpdate() const override
		{
			return true;
		}
		
	public:
        SShaderEditorBox* OwnerWidget;
		FTextLayout* TextLayout;
		TSharedPtr<HlslTokenizer> Tokenizer;
		TArray<HlslTokenizer::TokenizedLine> TokenizedLines;
		//Key: The line index of Left Brace in MultiLineEditableText
		TMap<int32, HlslTokenizer::BracketGroup> FoldingBraceGroups;
		TArray<HlslTokenizer::BracketGroup> BracketGroups;
		int32 FontSize{};
	};

    class TokenBreakIterator : public IBreakIterator
    {
    public:
        TokenBreakIterator(FShaderEditorMarshaller* InMarshaller);

        virtual void SetString(FString&& InString) override;
        virtual void SetStringRef(FStringView InString) override;

        virtual int32 GetCurrentPosition() const override;

        virtual int32 ResetToBeginning() override;
        virtual int32 ResetToEnd() override;

        virtual int32 MoveToPrevious() override;
        virtual int32 MoveToNext() override;
        virtual int32 MoveToCandidateBefore(const int32 InIndex) override;
        virtual int32 MoveToCandidateAfter(const int32 InIndex) override;
		
		bool IngoreWhitespace() override { return false; }

    private:
        FString InternalString;
        int32 CurrentPosition{};
        FShaderEditorMarshaller* Marshaller;
    };

    class ShaderTextLayout : public FSlateTextLayout
    {
    public:
        ShaderTextLayout(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle, FShaderEditorMarshaller* Marshaller)
            :FSlateTextLayout(InOwner, InDefaultTextStyle)
        {
            WordBreakIterator = MakeShared<TokenBreakIterator>(Marshaller);
        }
    };

	class FShaderEditorEffectMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FShaderEditorEffectMarshaller(SShaderEditorBox* InOwnerWidget) 
			: OwnerWidget(InOwnerWidget)
			, TextLayout(nullptr) 
		{}

	public:
		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
		
		void SubmitEffectText();
        
    public:
        struct DiagEffectInfo
        {
			bool IsError{};
			bool IsWarn{};
            FTextRange DummyRange;
            FTextRange InfoRange;
            FString TotalInfo;
        };
		
	public:
		SShaderEditorBox* OwnerWidget;
		FTextLayout* TextLayout;
		TMap<int32, DiagEffectInfo> LineNumberToDiagInfo;
	};

	struct ISenseTask
	{
		std::optional<FW::GpuShaderSourceDesc> ShaderDesc;

		FString CursorToken;
		uint32 Row = 0;
		uint32 Col = 0;
		bool IsMemberAccess = false;
	};

	struct SyntaxTask
	{
		std::optional<FW::GpuShaderSourceDesc> ShaderDesc;
		TArray<TArray<HlslTokenizer::Token>> LineTokens;
	};

	class SShaderMultiLineEditableText : public SMultiLineEditableText
	{
	public:
		void Construct(const FArguments& InArgs, SShaderEditorBox* InOwner)
		{
			Owner = InOwner;
			SMultiLineEditableText::Construct(InArgs);
		}
		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	private:
		SShaderEditorBox* Owner = nullptr;
	};

	//LineNumber: the line number of the visible shader source in editor
	//LineIndex: the index in SMultiLineEditableText
	//Line: the line number in the full shader source
	class SShaderEditorBox : public SCompoundWidget
	{
	public:
		using LineNumberItemPtr = TSharedPtr<FText>;
        using CandidateItemPtr = TSharedPtr<FW::ShaderCandidateInfo>;
        
        enum class EditState
        {
            Succeed,
			Editing,
            Compiling,
            Failed,
        };
		
		enum class StepMode
		{
			None,
			StepOver,
			StepInto,
		};
		
		static FTextBlockStyle& GetTokenStyle(HLSL::TokenType InType);
		static FSlateFontInfo& GetCodeFontInfo();
		
    public:
		SLATE_BEGIN_ARGS(SShaderEditorBox) 
			: _ShaderAssetObj(nullptr)
		{}
			SLATE_ARGUMENT(ShaderAsset*, ShaderAssetObj)
		SLATE_END_ARGS()

		SShaderEditorBox() = default;
		~SShaderEditorBox();
		
		void Construct(const FArguments& InArgs);

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		void OnShaderTextChanged(const FString& InShaderSouce);
        
        virtual bool SupportsKeyboardFocus() const override
        {
            return true;
        }
        
		void OnCursorMoved(const FTextLocation& NewCursorLocation);
        virtual void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
        virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
        void SetFocus() {
            FSlateApplication::Get().SetUserFocus(0, ShaderMultiLineEditableText);
        }
        
        //If type a char, trigger KeyDown and then KeyChar.
		FReply HandleKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent);
        FReply HandleKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
		TSharedRef<ITableRow> GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<ITableRow> GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
        TSharedRef<ITableRow> GenerateCodeCompletionItem(CandidateItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		bool IsErrorLine(int InLineNumber) const;
		bool IsWarningLine(int InLineNumber) const;

		ShaderAsset* GetShaderAsset() const { return ShaderAssetObj; }
        FText GetEditStateText() const;
        FText GetFontSizeText() const;
        FText GetRowColText() const;
        FSlateColor GetEditStateColor() const;
		int32 GetLineNumber(int32 InLineIndex) const;
		int32 GetLineIndex(int32 InLineNumber) const;
		int32 GetCurDisplayLineCount() const { return ShaderMarshaller->TextLayout->GetLineCount(); }
        void Compile();

		//Scroll to the specified line, which will be displayed in the middle of the document if possible.
		void ScrollTo(int32 InLineIndex);
		TOptional<int32> FindFoldMarker(int32 InIndex) const;
		
		//Given a range for displayed text, then return the unfolding result.
		FString UnFoldText(const FTextSelection& DisplayedTextRange);
		void UnFold(int32 LineNumber);
		void Fold(int32 LineNumber);
		void ToggleComment();
        
        void UpdateLineNumberData();
        void RefreshLineNumberToDiagInfo();
        void RefreshCodeCompletionTip();
		void RefreshSyntaxHighlight();
		void RefreshOccurrenceHighlight();
		void RefreshBracketHighlight();
		
		void DebugPixel(const FW::Vector2u& PixelCoord, const TArray<TRefCountPtr<FW::GpuBindGroup>>& BindGroups = {});
		void ApplyDebugState(const FW::SpvDebugState& State, bool bReverse = false);
		void ShowDeuggerVariable(FW::SpvLexicalScope* InScope) const;
		void ShowDebuggerResult() const;
		struct ExpressionNode EvaluateExpression(const FString& InExpression) const;
		void Continue(StepMode Mode = StepMode::None);
		void ClearDebugger();

	protected:
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
		void OnBeginEditTransaction();
        
	private:
		void CopySelectedText();
		void CutSelectedText();
        void PasteText();

		void UpdateEffectText();
		void HandleAutoIndent();
        TSharedRef<SWidget> BuildInfoBar();
		void GenerateInfoBarBox();

		FReply OnFold(int32 LineNumber);
		void RemoveFoldMarker(int32 InIndex);
        
        void InsertCompletionText(CandidateItemPtr InItem);

	public:
        FSlateEditableTextLayout* ShaderMultiLineEditableTextLayout;
		TArray<FoldMarker> VisibleFoldMarkers;
		TArray<int32> BreakPointLines;
        //The visible content in editor, and contains fold markers.
        FString CurrentEditorSource;
		const FSlateBrush* BackgroundLayerBrush{};
		TSharedPtr<FShaderEditorMarshaller> ShaderMarshaller;

		//Syntax highlight
		FW::ShaderTU SyntaxTU;
		TArray<TMap<FTextRange, FTextBlockStyle*>> LineSyntaxHighlightMaps;
		TArray<TMap<FTextRange, FTextBlockStyle*>> LineSyntaxHighlightMapsCopy;
		TUniquePtr<FThread> SyntaxThread;
		TQueue<SyntaxTask> SyntaxQueue;
		std::atomic<bool> bQuitISyntax{};
		std::atomic<bool> bRefreshSyntax{};
		FEvent* SyntaxEvent = nullptr;
		//

		//Debugger
		TArray<FW::ShaderFunc> Funcs;
		TArray<TPair<FW::SpvLexicalScope*, int>> CallStack;
		FW::SpvLexicalScope* Scope = nullptr;
		FW::SpvLexicalScope* CallStackScope = nullptr;
		TMultiMap<FW::SpvId, FW::SpvVariableChange::DirtyRange> DirtyVars;
		int32 StopLineNumber{};
		//ValidLine: Line that can trigger a breakpoint
		std::optional<int32> CurValidLine;
		FString DebuggerError;
		int32 CurDebugStateIndex{};
		FW::SpvVmContext* DebuggerContext = nullptr;
		std::vector<std::pair<FW::SpvId, FW::SpvVariableDesc*>> SortedVariableDescs;
		std::optional<FW::SpvObject> CurReturnObject;
		FW::SpvVariable* AssertResult = nullptr;
		std::optional<FW::SpvVmPixelContext> VmPixelContext;
		bool bEditDuringDebugging{};
		//
        
	private:
        //The text after unfolding, but that may not be the content compiled finally.
		//The shader may contain extra declaration(eg. binding codegen)
		FString CurrentShaderSource;
        
		TMap<int32, int32> LineNumberToIndexMap;
		int32 MaxLineNumber{};
		TArray<LineNumberItemPtr> LineNumberData;
        TSharedPtr<SShaderMultiLineEditableText> ShaderMultiLineEditableText;
		TSharedPtr<FUICommandList> UICommandList;
        
        TSharedPtr<SScrollBar> ShaderMultiLineVScrollBar;
        TSharedPtr<SScrollBar> ShaderMultiLineHScrollBar;

		TArray<FTextLineHighlight> OccurrenceHighlights;
		TOptional<FTextLineHighlight> OpenBracketHighlight, CloseBracketHighlight;
        TSharedPtr<class CursorHighlighter> CustomCursorHighlighter;
        
		TSharedPtr<FShaderEditorEffectMarshaller> EffectMarshller;
		TSharedPtr<SMultiLineEditableText> EffectMultiLineEditableText;
        
		TSharedPtr<SListView<LineNumberItemPtr>> LineNumberList;
		TSharedPtr<SListView<LineNumberItemPtr>> LineTipList;
		
		FTextSelection SelectionBeforeEdit;

		ShaderAsset* ShaderAssetObj;
		FDelegateHandle RefreshBuilderHandle;
        EditState CurEditState;
		TSharedPtr<SHorizontalBox> InfoBarBox;

		bool IsFoldEditTransaction{};
		FCurveSequence FoldingArrowAnim;
        
        //CodeComplete and real-time diagnostic
		FW::ShaderTU ISenseTU;
		TUniquePtr<FThread> ISenseThread;
		TQueue<ISenseTask> ISenseQueue;

		std::atomic<bool> bQuitISense{};
		std::atomic<bool> bRefreshIsense{};
		FEvent* ISenseEvent = nullptr;
        TArray<FW::ShaderDiagnosticInfo> DiagnosticInfos;
        FString CurToken;
        TArray<FW::ShaderCandidateInfo> CandidateInfos;
		TSharedPtr<SCanvas> CodeCompletionCanvas;
        TSharedPtr<SListView<CandidateItemPtr>> CodeCompletionList;
        CandidateItemPtr CurSelectedCandidate;
        TArray<CandidateItemPtr> CandidateItems;
        bool bTryComplete = false;
        bool bKeyChar = false;
        //
		bool bTryMergeUndoState = false;
		
	};
}
