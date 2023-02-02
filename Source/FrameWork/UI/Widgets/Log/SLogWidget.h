#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Input/SMultiLineEditableTextBox.h>
#include <Widgets/Input/SSearchBox.h>

namespace FRAMEWORK
{

	class FLogWidgetMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FLogWidgetMarshaller();
		~FLogWidgetMarshaller();

		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
	private:
		FTextLayout* TextLayout;
	};

	struct FLogFilter
	{
		bool bShowLogs;
		bool bShowWarnings;
		bool bShowErrors;
		bool bShowAllCategories;

		TArray<FName> AvailableLogCategories;
		TArray<FName> SelectedLogCategories;
	};

	class FRAMEWORK_API SLogWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLogWidget) {}
			SLATE_ARGUMENT(TSharedPtr<ITextLayoutMarshaller>, Marshaller)
			SLATE_ARGUMENT(TSharedPtr<FLogFilter>, Filter)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		virtual void OnFilterTextChanged(const FText& InFilterText);
		virtual void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
		virtual TSharedRef<SWidget> MakeAddFilterMenu();

	protected:
		TSharedPtr<SMultiLineEditableTextBox> MessagesTextBox;
		TSharedPtr<SSearchBox> SearchBox;
		TSharedPtr<ITextLayoutMarshaller> Marshaller;
	};
}


