#pragma once
#include <Widgets/Input/SMultiLineEditableTextBox.h>
#include "SLogWidget.h"
namespace FRAMEWORK 
{
	/**
	* A single log message for the output log, holding a message and
	* a style, for color and bolding of the message.
	*/
	struct FOutputLogMessage
	{
		TSharedRef<FString> Message;
		ELogVerbosity::Type Verbosity;
		int8 CategoryStartIndex;
		FName Category;
		FName Style;

		FOutputLogMessage(const TSharedRef<FString>& NewMessage, ELogVerbosity::Type NewVerbosity, FName NewCategory, FName NewStyle, int32 InCategoryStartIndex)
			: Message(NewMessage)
			, Verbosity(NewVerbosity)
			, CategoryStartIndex((int8)InCategoryStartIndex)
			, Category(NewCategory)
			, Style(NewStyle)
		{
		}
	};

	class FOutputLogMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FOutputLogMarshaller();
		~FOutputLogMarshaller();

		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
	private:
		FTextLayout* TextLayout;
	};

	class FRAMEWORK_API SOutputLog : public SLogWidget, public FOutputDevice
	{
	public:
		SLATE_BEGIN_ARGS(SOutputLog) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		
		virtual void OnFilterTextChanged(const FText& InFilterText) override;
		virtual void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType) override;
		virtual TSharedRef<SWidget> MakeAddFilterMenu() override;
		
	protected:
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;
	};
}

