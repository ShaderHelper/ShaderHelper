#include "CommonHeader.h"
#include "SCodeSearchWidget.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "Editor/CodeEditorCommands.h"

using namespace FW;

namespace SH
{

	void SCodeSearchWidget::Construct(const FArguments& InArgs)
	{
		OnTextChanged = InArgs._OnTextChanged;
		OnReplaced = InArgs._OnReplaced;
		OnClosed = InArgs._OnClosed;
		OnMatchCaseChanged = InArgs._OnMatchCaseChanged;
		OnMatchWholeChanged = InArgs._OnMatchWholeChanged;
		OnSearchDelegate = InArgs._OnResultNavigationButtonClicked;
		SearchResultData = InArgs._SearchResultData;
		UICommandList = InArgs._UICommandList;

		ChildSlot
		[
			SNew(SBorder)
			[
				SNew(SGridPanel)
				+ SGridPanel::Slot(0, 0)
				[
					SNew(SIconButton).Icon_Lambda([this] {
						return IsReplaceable ? FAppStyle::Get().GetBrush("Icons.ChevronDown") : FAppStyle::Get().GetBrush("Icons.ChevronRight");
					})
					.OnClicked_Lambda([this] {
						IsReplaceable = !IsReplaceable;
						return FReply::Handled();
					})
				]
				+ SGridPanel::Slot(1, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SGridPanel)
						+SGridPanel::Slot(0,0)
						[

							SAssignNew(SearchEditableText, SEditableTextBox)
							.RevertTextOnEscape(false)
							.ClearKeyboardFocusOnCommit(false)
							.HintText(LOCALIZATION("Search"))
							.SelectAllTextWhenFocused(true)
							.OnTextChanged_Lambda([this](const FText& InText) {
								LastSearchedText = InText;
								OnTextChanged.ExecuteIfBound(InText);
							})
							.MinDesiredWidth(150)
							.OnKeyDownHandler_Lambda([this](const FGeometry&, const FKeyEvent& InKeyEvent) {
								if (InKeyEvent.GetKey() == EKeys::Enter)
								{
									OnSearchDelegate.ExecuteIfBound(InKeyEvent.GetModifierKeys().IsShiftDown() ? SSearchBox::Previous : SSearchBox::Next);
									return FReply::Handled();
								}
								return FReply::Unhandled();
							})
													
						]
						+ SGridPanel::Slot(0, 1)
						.Padding(0.f, 2.f, 0.f, 0.f)
						[
							SAssignNew(ReplaceEditableText, SEditableTextBox)
							.RevertTextOnEscape(false)
							.ClearKeyboardFocusOnCommit(false)
							.Visibility_Lambda([this] { return IsReplaceable ? EVisibility::Visible : EVisibility::Collapsed; })
							.HintText(LOCALIZATION("Replace"))
							.MinDesiredWidth(150)
							.OnKeyDownHandler_Lambda([this](const FGeometry&, const FKeyEvent& InKeyEvent) {
								if (InKeyEvent.GetKey() == EKeys::Enter)
								{
									OnReplaced.ExecuteIfBound(ReplaceEditableText->GetText(), false);
									return FReply::Handled();
								}
								return FReply::Unhandled();
							})
						]
						+ SGridPanel::Slot(1, 1)
						[
							SNew(SHorizontalBox)
							.Visibility_Lambda([this] {  return IsReplaceable ? EVisibility::Visible : EVisibility::Collapsed; })
							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SIconButton).Icon(FShaderHelperStyle::Get().GetBrush("Icons.Replace")).IsFocusable(false)
								.ToolTipText(LOCALIZATION("Replace"))
								.OnClicked_Lambda([this] {
									OnReplaced.ExecuteIfBound(ReplaceEditableText->GetText(), false);
									return FReply::Handled();
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SIconButton).Icon(FShaderHelperStyle::Get().GetBrush("Icons.ReplaceAll")).IsFocusable(false)
								.ToolTipText(LOCALIZATION("ReplaceAll"))
								.OnClicked_Lambda([this] {
									OnReplaced.ExecuteIfBound(ReplaceEditableText->GetText(), true);
									return FReply::Handled();
								})
							]
						]
						+ SGridPanel::Slot(1, 0)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.ArrowUp")).IsFocusable(false)
								.OnClicked_Lambda([this] {
									OnSearchDelegate.ExecuteIfBound(SSearchBox::Previous);
									return FReply::Handled();
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("Icons.ArrowDown")).IsFocusable(false)
								.OnClicked_Lambda([this] {
									OnSearchDelegate.ExecuteIfBound(SSearchBox::Next);
									return FReply::Handled();
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SIconButton).Icon(FAppCommonStyle::Get().GetBrush("Icons.Close"))
								.OnClicked_Lambda([this] {
									OnClosed.ExecuteIfBound();
									SetVisibility(EVisibility::Collapsed);
									return FReply::Handled();
								})
							]
						]
					
					]
				]
				+ SGridPanel::Slot(1, 1)
				.Padding(0, 2.0f, 0, 0)
				.HAlign(HAlign_Left)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 8.0f, 0)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text_Lambda([this] {
							const auto& Data = SearchResultData.Get({});
							return FText::FromString(FString::Printf(TEXT("%d / %d"), Data.CurrentSearchResultIndex, Data.NumSearchResults));
						})
					]
					+ SHorizontalBox::Slot()
					.Padding(0, 0, 4.0f, 0)
					.AutoWidth()
					[
						SNew(SShToggleButton).Icon(FShaderHelperStyle::Get().GetBrush("Icons.MatchCase"))
						.ToolTipText(LOCALIZATION("MatchCase"))
						.IsChecked_Lambda([this] { return bMatchCase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
							bMatchCase = InState == ECheckBoxState::Checked ? true : false;
							OnMatchCaseChanged.ExecuteIfBound(bMatchCase);
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SShToggleButton).Icon(FShaderHelperStyle::Get().GetBrush("Icons.MatchWhole"))
						.ToolTipText(LOCALIZATION("MatchWhole"))
						.IsChecked_Lambda([this] { return bMatchWhole ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
							bMatchWhole = InState == ECheckBoxState::Checked ? true : false;
							OnMatchWholeChanged.ExecuteIfBound(bMatchWhole);
						})
					]
				]
			]
		];
	}

	void SCodeSearchWidget::TriggerSearch(const FText& InNewSearchText, bool IsReplaceable)
	{
		this->IsReplaceable = IsReplaceable;

		// multiline search is not supported, sanitize the input to be single line text
		FString SingleLineString = InNewSearchText.ToString();
		{
			SingleLineString.GetCharArray().RemoveAll([&](const TCHAR InChar) -> bool
				{
					if (InChar != 0)
					{
						const bool bIsCharAllowed = !FChar::IsLinebreak(InChar);
						return !bIsCharAllowed;
					}
					return false;
				});
		}

		FText SingleLineSearchText = FText::FromString(SingleLineString);

		// clear the text to trigger a fresh search
		// sometimes, the search text can be the same but starting from different place
		FText SearchedText = SearchEditableText->GetText();
		SearchEditableText->SetText(FText::GetEmpty());
		LastSearchedText = SearchedText;

		if (InNewSearchText.IsEmpty())
		{
			SearchEditableText->SetText(LastSearchedText);
		}
		else
		{
			SearchEditableText->SetText(SingleLineSearchText);
		}

		if (IsReplaceable)
		{
			FSlateApplication::Get().SetKeyboardFocus(ReplaceEditableText, EFocusCause::SetDirectly);
		}
		else
		{
			SearchEditableText->SelectAllText();
			FSlateApplication::Get().SetKeyboardFocus(SearchEditableText, EFocusCause::SetDirectly);
		}
	}

	FReply SCodeSearchWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (UICommandList && UICommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
}