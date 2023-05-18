#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Views/SListView.h>
#include <Widgets/Text/SMultiLineEditableText.h>
#include "UI/ShaderHelperWindow/ShaderCodeEditor/ShaderCodeTokenizer.h"

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

	class SShaderEditorBox : public SCompoundWidget
	{
	public:
		using LineNumberItemPtr = TSharedPtr<FText>;

		SLATE_BEGIN_ARGS(SShaderEditorBox) {}
			SLATE_ARGUMENT(FText, Text)
		SLATE_END_ARGS()
		
		void Construct(const FArguments& InArgs);

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

		void OnShaderTextChanged(const FText& InText);
		void OnShadedrTextCommitted(const FText& Name, ETextCommit::Type CommitInfo);
		FReply OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) const;
		TSharedRef<ITableRow> GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<ITableRow> GenerateRowTipForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

		FText GetShadedrCode() const;
		
	private:
		void UpdateLineTipStyle(const double InCurrentTime);
		void UpdateLineNumberHighlight();
		void UpdateLineNumberAndTipListViewScrollBar();
		void HandleAutoIndent() const;

	private:
		int32 CurLineNum;
		TArray<LineNumberItemPtr> LineNumberData;
		TSharedPtr<FShaderEditorMarshaller> Marshaller;
		TSharedPtr<SMultiLineEditableText> ShaderMultiLineEditableText;
		TSharedPtr<SListView<LineNumberItemPtr>> LineNumberList;
		TSharedPtr<SListView<LineNumberItemPtr>> LineTipList;
		FString ShaderCode;

		TSharedPtr<SScrollBar> ShaderMultiLineVScrollBar;
	};
}