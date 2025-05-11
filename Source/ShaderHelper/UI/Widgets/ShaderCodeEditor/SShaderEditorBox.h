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

namespace SH
{
    class SShaderEditorBox;
    class StShader;

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
        int32 GetFoledLineCounts() const;
    };

    struct ShaderUndoState : SlateEditableTextTypes::FUndoState
    {
        TArray<FoldMarker> FoldMarkers;
    };

	class FShaderEditorMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<HlslHighLightTokenizer> InTokenizer);
		
	public:
		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

		//ReTokenize when the text changed.
		bool RequiresLiveUpdate() const override
		{
			return true;
		}
		
	public:
        SShaderEditorBox* OwnerWidget;
		FTextLayout* TextLayout;
		TSharedPtr<HlslHighLightTokenizer> Tokenizer;
		TMap<HlslHighLightTokenizer::TokenType, FTextBlockStyle> TokenStyleMap;
		//Key: The line index of Left Brace in MultiLineEditableText
		TMap<int32, HlslHighLightTokenizer::BraceGroup> FoldingBraceGroups;
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
		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
		
		void SubmitEffectText();
        
    public:
        struct ErrorEffectInfo
        {
            FTextRange DummyRange;
            FTextRange ErrorRange;
            FString TotalInfo;
        };
		
	public:
		SShaderEditorBox* OwnerWidget;
		FTextLayout* TextLayout;
		TMap<int32, ErrorEffectInfo> LineNumberToErrorInfo;
	};

	class SShaderEditorBox : public SCompoundWidget
	{
	public:
		using LineNumberItemPtr = TSharedPtr<FText>;
        using CandidateItemPtr = TSharedPtr<FW::ShaderCandidateInfo>;
        
        enum class EditState
        {
            Succeed,
            Compiling,
            Failed,
        };
        
        struct ISenseTask
        {
            TRefCountPtr<FW::GpuShader> Shader;
            
            FString CursorToken;
            uint32 Row = 0;
            uint32 Col = 0;
            bool IsMemberAccess = false;
        };
        
    public:
		SLATE_BEGIN_ARGS(SShaderEditorBox) 
			: _StShaderAsset(nullptr)
		{}
			SLATE_ARGUMENT(StShader*, StShaderAsset)
		SLATE_END_ARGS()

		~SShaderEditorBox();
		
		void Construct(const FArguments& InArgs);

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		void OnShaderTextChanged(const FString& InShaderSouce);
        
        virtual bool SupportsKeyboardFocus() const override
        {
            return true;
        }
        
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

        const FSlateFontInfo& GetFontInfo() const { return CodeFontInfo; }
        FText GetEditStateText() const;
        FText GetFontSizeText() const;
        FText GetRowColText() const;
        FSlateColor GetEditStateColor() const;
		int32 GetLineNumber(int32 InLineIndex) const;
		int32 GetLineIndex(int32 InLineNumber) const;
		int32 GetCurDisplayLineCount() const { return ShaderMarshaller->TextLayout->GetLineCount(); }
        void Compile();

		TOptional<int32> FindFoldMarker(int32 InIndex) const;
		
		//Given a range for displayed text, then return the unfolding result.
		FString UnFoldText(const FTextSelection& DisplayedTextRange);
        
        void UpdateLineNumberData();
        void RefreshLineNumberToErrorInfo();
        void RefreshCodeCompletionTip();

	protected:
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
        
	private:
		void CopySelectedText();
		void CutSelectedText();
        void PasteText();

		void UpdateListViewScrollBar();
		void UpdateEffectText();
		void HandleAutoIndent();
        TSharedRef<SWidget> BuildInfoBar();
		void GenerateInfoBarBox();

		FReply OnFold(int32 LineNumber);
		void RemoveFoldMarker(int32 InIndex);
        
        void InsertCompletionText(const FString& InText);

	public:
        FSlateEditableTextLayout* ShaderMultiLineEditableTextLayout;
		TArray<FoldMarker> VisibleFoldMarkers;
        //Contain fold markers.
        FString CurrentEditorSource;
        
	private:
        //The text after unfolding.
		FString CurrentShaderSource;
        
		TArray<LineNumberItemPtr> LineNumberData;
		TSharedPtr<FShaderEditorMarshaller> ShaderMarshaller;
        TSharedPtr<SMultiLineEditableText> ShaderMultiLineEditableText;
        
        TSharedPtr<SScrollBar> ShaderMultiLineVScrollBar;
        TSharedPtr<SScrollBar> ShaderMultiLineHScrollBar;
        TSharedPtr<class CursorHightLighter> CustomCursorHighlighter;
        
		TSharedPtr<FShaderEditorEffectMarshaller> EffectMarshller;
		TSharedPtr<SMultiLineEditableText> EffectMultiLineEditableText;
        
		TSharedPtr<SListView<LineNumberItemPtr>> LineNumberList;
		TSharedPtr<SListView<LineNumberItemPtr>> LineTipList;

		StShader* StShaderAsset;
        EditState CurEditState;
        FSlateFontInfo CodeFontInfo;
		TSharedPtr<SHorizontalBox> InfoBarBox;

		FCurveSequence FoldingArrowAnim;
        
        //CodeComplete and real-time diagnostic
        TUniquePtr<FThread> ISenseThread;
        TQueue<ISenseTask> ISenseQueue;
        std::atomic<bool> bQuitISense{};
        std::atomic<bool> bRefreshIsense{};
        FEvent* ISenseEvent = nullptr;
        TArray<FW::ShaderErrorInfo> ErrorInfos;
        
        FString CurToken;
        TArray<FW::ShaderCandidateInfo> CandidateInfos;
		TSharedPtr<SCanvas> CodeCompletionCanvas;
        TSharedPtr<SListView<CandidateItemPtr>> CodeCompletionList;
        CandidateItemPtr CurSelectedCandidate;
        TArray<CandidateItemPtr> CandidateItems;
        bool bTryComplete = false;
        bool bKeyChar = false;
        //
	};
}
