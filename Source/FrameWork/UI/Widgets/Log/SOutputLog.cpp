#include "CommonHeader.h"
#include "SOutputLog.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK 
{
	void SOutputLog::Construct(const FArguments& InArgs)
	{
		/*SLogWidget::Construct(
			SLogWidget::FArguments().
		);*/
	}

	void SOutputLog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{

	}

	void SOutputLog::OnFilterTextChanged(const FText& InFilterText)
	{

	}

	void SOutputLog::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
	{

	}

	TSharedRef<SWidget> SOutputLog::MakeAddFilterMenu()
	{
		FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);
		return MakeShared<SBox>();
	}

	void SOutputLog::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
	{

	}

	FOutputLogMarshaller::FOutputLogMarshaller()
	{

	}

	FOutputLogMarshaller::~FOutputLogMarshaller()
	{

	}

	void FOutputLogMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{

	}

	void FOutputLogMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{

	}

}