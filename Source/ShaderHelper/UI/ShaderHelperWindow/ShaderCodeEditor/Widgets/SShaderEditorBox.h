#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/ShaderHelperWindow/ShaderCodeEditor/ShaderCodeTokenizer.h"
#include "Renderer/ShRenderer.h"

namespace SH
{
	class FShaderEditorMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FShaderEditorMarshaller(TSharedPtr<HlslHighLightTokenizer> InTokenizer);
		
	public:
		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

		//ReTokenize when the text changed.
		bool RequiresLiveUpdate() const override
		{
			return true;
		}
		
	public:
		FTextLayout* TextLayout;
		TSharedPtr<HlslHighLightTokenizer> Tokenizer;
		TMap<HlslHighLightTokenizer::TokenType, FTextBlockStyle> TokenStyleMap;
	};

	class SShaderEditorBox;

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
		SShaderEditorBox* OwnerWidget;
		FTextLayout* TextLayout;
		TMap<int32, FString> LineNumToErrorInfo;
	};

	class SShaderEditorBox : public SCompoundWidget
	{
	public:
		using LineNumberItemPtr = TSharedPtr<FText>;

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
		TSharedRef<ITableRow> GenerateRowTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

		int32 GetCurLineNum() const { return CurLineNum; }

	private:
		void UpdateLineTipStyle(const double InCurrentTime);
		void UpdateLineNumberHighlight();
		void UpdateListViewScrollBar();
		void UpdateEffectText();
		void HandleAutoIndent() const;

	private:
		int32 CurLineNum;
		TArray<LineNumberItemPtr> LineNumberData;
		TSharedPtr<FShaderEditorMarshaller> ShaderMarshaller;
		TSharedPtr<FShaderEditorEffectMarshaller> EffectMarshller;
		TSharedPtr<SMultiLineEditableText> ShaderMultiLineEditableText;
		TSharedPtr<SMultiLineEditableText> EffectMultiLineEditableText;
		TSharedPtr<SListView<LineNumberItemPtr>> LineNumberList;
		TSharedPtr<SListView<LineNumberItemPtr>> LineTipList;
		TSharedPtr<SScrollBar> ShaderMultiLineVScrollBar;
		TSharedPtr<SScrollBar> ShaderMultiLineHScrollBar;
		ShRenderer* Renderer;
	};
}