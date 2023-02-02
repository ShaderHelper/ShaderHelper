#include "CommonHeader.h"
#include "SLogWidget.h"
#include "UI/Styles/FAppCommonStyle.h"

#define LOCTEXT_NAMESPACE "SLogWidget"

namespace FRAMEWORK
{
	FLogWidgetMarshaller::FLogWidgetMarshaller()
		:TextLayout(nullptr)
	{

	}

	FLogWidgetMarshaller::~FLogWidgetMarshaller()
	{

	}

	void FLogWidgetMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;
	}

	void FLogWidgetMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	void SLogWidget::Construct(const FArguments& InArgs)
	{
		Marshaller = InArgs._Marshaller ? InArgs._Marshaller : MakeShared<FLogWidgetMarshaller>();

		MessagesTextBox = SNew(SMultiLineEditableTextBox)
			.Style(FAppCommonStyle::Get(), "Log.TextBox")
			.TextStyle(FAppCommonStyle::Get(), "Log.Normal")
			.Marshaller(Marshaller)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true);
		
		TSharedPtr<SVerticalBox> SBox = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				MessagesTextBox.ToSharedRef()
			];
		
		ChildSlot
		.Padding(3)
		[
			SBox.ToSharedRef()
		];

		if (InArgs._Filter) {
			SBox->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0, 0, 4, 0)
				.FillWidth(.65f)
				[
					SAssignNew(SearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchLogHint", "Search Log"))
					.OnTextChanged_Lambda(
						[this](const FText& InFilterText) { OnFilterTextChanged(InFilterText); }
					)
					.OnTextCommitted_Lambda(
						[this](const FText& InFilterText, ETextCommit::Type InCommitType) { OnFilterTextCommitted(InFilterText, InCommitType); }
					)
					.DelayChangeNotificationsWhileTyping(true)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					SNew(SComboButton)
					.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
					.ToolTipText(LOCTEXT("AddFilterToolTip", "Add an output log filter."))
					.OnGetMenuContent_Lambda(
						[this] { return MakeAddFilterMenu(); }
					)
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 0, 0)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Filters", "Filters"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
				]
			];
		}
	}


	void SLogWidget::OnFilterTextChanged(const FText& InFilterText)
	{

	}

	void SLogWidget::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
	{

	}

	TSharedRef<SWidget> SLogWidget::MakeAddFilterMenu()
	{
		//TODO
		return MakeShared<SBox>();
	}

}
