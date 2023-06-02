#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/ShaderHelperWindow/ShaderCodeEditor/ShaderCodeTokenizer.h"
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
		TMap<int32, ErrorEffectInfo> LineNumToErrorInfo;
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
        
    public:
		SLATE_BEGIN_ARGS(SShaderEditorBox) 
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(FText, Text)
			SLATE_ARGUMENT(ShRenderer*, Renderer)
		SLATE_END_ARGS()
		
		void Construct(const FArguments& InArgs);

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

		void OnShaderTextChanged(const FText& InText);
		FReply OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) const;
		TSharedRef<ITableRow> GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<ITableRow> GenerateLineTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

		int32 GetCurLineNum() const { return CurLineNum; }
        const FSlateFontInfo& GetFontInfo() const { return CodeFontInfo; }
        FText GetEditStateText() const;
        FText GetFontSizeText() const;
        FText GetRowColText() const;
        FSlateColor GetEditStateColor() const;
        
	private:
		void UpdateLineTipStyle(const double InCurrentTime);
		void UpdateLineNumberHighlight();
		void UpdateListViewScrollBar();
		void UpdateEffectText();
		void HandleAutoIndent() const;
        TSharedRef<SWidget> BuildInfoBar();
    
	private:
		int32 CurLineNum;
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

	};
}
