#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/Widgets/ShaderCodeEditor/ShaderCodeTokenizer.h"
#include "Renderer/ShRenderer.h"

namespace SH
{
    class SShaderEditorBox;

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
        enum class EditState
        {
            Normal,
            Compiling,
            Failed,
        };

		struct FoldMarker
		{
			int32 LineIndex;
			int32 Offset;
			TArray<FString> FoldedLineTexts;
			//Note: ChildFoldMarker's line index and offset is invalid.
			TArray<FoldMarker> ChildFoldMarkers;

			FString GetTotalFoldedLineTexts() const;
			int32 GetFoledLineCounts() const;
		};
        
    public:
		SLATE_BEGIN_ARGS(SShaderEditorBox) 
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(FText, Text)
			SLATE_ARGUMENT(ShRenderer*, Renderer)
		SLATE_END_ARGS()
		
		void Construct(const FArguments& InArgs);

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

		bool OnShaderTextChanged(const FString& NewShaderSouce);
		FReply OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent);
		TSharedRef<ITableRow> GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<ITableRow> GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

        const FSlateFontInfo& GetFontInfo() const { return CodeFontInfo; }
        FText GetEditStateText() const;
        FText GetFontSizeText() const;
        FText GetRowColText() const;
        FSlateColor GetEditStateColor() const;
		int32 GetLineNumber(int32 InLineIndex) const;
		int32 GetLineIndex(int32 InLineNumber) const;
		int32 GetCurDisplayLineCount() const { return ShaderMarshaller->TextLayout->GetLineCount(); }
		FString GetCurFullShaderSource() const { return CurrentFullShaderSource; }
		bool ReCompile() { return OnShaderTextChanged(CurrentShaderSource); }

		TOptional<int32> FindFoldMarker(int32 InIndex) const;
		void MarkLineNumberDataDirty(bool IsDirty) { IsLineNumberDataDirty = IsDirty; }
		
		//Given a range for displayed text, then return the unfolding result.
		FString UnFold(const FTextSelection& DisplayedTextRange);

	protected:
		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
        
	private:
		void CopySelectedText();
		bool CanCopySelectedText() const;
		void CutSelectedText();
		bool CanCutSelectedText() const;

		void UpdateLineNumberData();
		void UpdateLineTipStyle(const double InCurrentTime);
		void UpdateLineNumberHighlight();
		void UpdateListViewScrollBar();
		void UpdateEffectText();
		void UpdateFoldingArrow();
		void HandleAutoIndent();
        TSharedRef<SWidget> BuildInfoBar();
		void GenerateInfoBarBox();

		FReply OnFold(int32 LineNumber);
		void RemoveFoldMarker(int32 InIndex);

	public:
		TArray<FoldMarker> DisplayedFoldMarkers;
    
	private:
		FString CurrentShaderSource;
		FString CurrentFullShaderSource;

		bool IsLineNumberDataDirty = false;
		TArray<LineNumberItemPtr> LineNumberData;
		TSharedPtr<FShaderEditorMarshaller> ShaderMarshaller;
        TSharedPtr<SMultiLineEditableText> ShaderMultiLineEditableText;
        TSharedPtr<SScrollBar> ShaderMultiLineVScrollBar;
        TSharedPtr<SScrollBar> ShaderMultiLineHScrollBar;
        
		TSharedPtr<FShaderEditorEffectMarshaller> EffectMarshller;
		TSharedPtr<SMultiLineEditableText> EffectMultiLineEditableText;
        
		TSharedPtr<SListView<LineNumberItemPtr>> LineNumberList;
		TSharedPtr<SListView<LineNumberItemPtr>> LineTipList;

		ShRenderer* Renderer;
        EditState CurEditState;
        FSlateFontInfo CodeFontInfo;
		TSharedPtr<SHorizontalBox> InfoBarBox;

		FCurveSequence FoldingArrowAnim;
	};
}
