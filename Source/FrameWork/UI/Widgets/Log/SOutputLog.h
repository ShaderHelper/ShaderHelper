#pragma once
#include "SLogWidget.h"

#include <Widgets/Input/SMultiLineEditableTextBox.h>
namespace FW 
{
	/**
	* A single log message for the output log, holding a message and
	* a style, for color and bolding of the message.
	*/
	struct FOutputLogMessage
	{
		TSharedPtr<FString> Message;
		ELogVerbosity::Type Verbosity;
		FName Category;
		FName Style;
	};

	struct FOutputLogFilter
	{
		void AddAvailableLogCategory(FName LogCategory);
		bool IsMessageAllowed(const FOutputLogMessage& Message);

		bool bShowLogs = true;
		bool bShowWarnings = true;
		bool bShowErrors = true;
		
		bool bShowUeLog = false;
		bool bShowShLog = true;

		TArray<FName> AvailableLogCategories;
		TArray<FName> SelectedLogCategories;
		FString FilterString;
	};

	class FOutputLogMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FOutputLogMarshaller(FOutputLogFilter* InFilter);

		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

		void AddMessage(FOutputLogMessage InMessage);
		bool SubmitPendingMessages();
		void AppendPendingMessagesToTextLayout();

		//Gets the num of messages added to TextLayout
		int32 GetNumMessages() const;
        void Clear();
	protected:
		int32 NextPendingMessageIndex;
		TArray<FOutputLogMessage> Messages;
		
		FOutputLogFilter* Filter;
		FTextLayout* TextLayout;
	};

	class FRAMEWORK_API SOutputLog : public SLogWidget, public FOutputDevice
	{
	public:
		SLATE_BEGIN_ARGS(SOutputLog) {}
		SLATE_END_ARGS()

		SOutputLog();
		~SOutputLog();

		void Construct(const FArguments& InArgs);
		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		
		void OnFilterTextChanged(const FText& InFilterText);
		void OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType);
		TSharedRef<SWidget> MakeAddFilterMenu();

		void VerbosityLogs_Execute();
		bool VerbosityLogs_IsChecked() const;

		void VerbosityWarnings_Execute();
		bool VerbosityWarnings_IsChecked() const;

		void VerbosityErrors_Execute();
		bool VerbosityErrors_IsChecked() const;

		void UeLog_Execute();
		bool UeLog_IsChecked() const;

		void ShLog_Execute();
		bool ShLog_IsChecked() const;

		void RequestForceScroll();
	protected:
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;
	protected:
		TSharedPtr<FOutputLogFilter> Filter;
		TSharedPtr<SSearchBox> SearchBox;
        TSharedPtr<FOutputLogMarshaller> OutPutLogMarshaller;
		bool bIsUserScrolled;
	};
}

