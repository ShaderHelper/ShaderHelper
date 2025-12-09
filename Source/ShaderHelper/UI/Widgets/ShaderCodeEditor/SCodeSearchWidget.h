#pragma once
#include <Widgets/Input/SSearchBox.h>
 
namespace SH
{
	DECLARE_DELEGATE_TwoParams(FOnReplaced, const FText&, bool);

	class SCodeSearchWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCodeSearchWidget) {}
			SLATE_EVENT(FOnTextChanged, OnTextChanged)
			SLATE_EVENT(FSimpleDelegate, OnClosed)
			SLATE_EVENT(FOnReplaced, OnReplaced)
			SLATE_EVENT(FOnBooleanValueChanged, OnMatchCaseChanged)
			SLATE_EVENT(FOnBooleanValueChanged, OnMatchWholeChanged)
			SLATE_EVENT(SSearchBox::FOnSearch, OnResultNavigationButtonClicked)
			SLATE_ARGUMENT(TSharedPtr<FUICommandList>, UICommandList)
			SLATE_ATTRIBUTE(SSearchBox::FSearchResultData, SearchResultData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void TriggerSearch(const FText& InNewSearchText, bool IsReplaceable) ;
		virtual bool SupportsKeyboardFocus() const override { return true; }
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
		FText GetSearchText() const { return SearchEditableText->GetText(); }

	private:
		FOnTextChanged OnTextChanged;
		FOnReplaced OnReplaced;
		FSimpleDelegate OnClosed;
		FOnBooleanValueChanged OnMatchCaseChanged, OnMatchWholeChanged;
		SSearchBox::FOnSearch OnSearchDelegate;
		TAttribute<SSearchBox::FSearchResultData> SearchResultData;
		TSharedPtr< SEditableTextBox > SearchEditableText;
		TSharedPtr< SEditableTextBox > ReplaceEditableText;
		FText LastSearchedText;
		bool IsReplaceable{};
		bool bMatchCase{}, bMatchWhole{};
		TSharedPtr<FUICommandList> UICommandList;
	};
}