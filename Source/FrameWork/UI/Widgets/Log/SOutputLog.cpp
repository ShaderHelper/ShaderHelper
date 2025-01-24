#include "CommonHeader.h"
#include "SOutputLog.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <Misc/OutputDeviceHelper.h>
#include <Framework/Text/SlateTextRun.h>
#include "UI/Widgets/Misc/SIconButton.h"

namespace FW 
{

	SOutputLog::SOutputLog()
		:bIsUserScrolled(false)
	{

	}

	SOutputLog::~SOutputLog()
	{
		if (GLog != nullptr)
		{
			GLog->RemoveOutputDevice(this);
		}
	}

	void SOutputLog::Construct(const FArguments& InArgs)
	{
		TSharedPtr<SHorizontalBox> FilterBox;
			
		SAssignNew(FilterBox, SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(0, 0, 4, 0)
		.FillWidth(.65f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText(FText::FromString("Search Log"))
			.OnTextChanged(this, &SOutputLog::OnFilterTextChanged)
			.OnTextCommitted(this, &SOutputLog::OnFilterTextCommitted)
			.DelayChangeNotificationsWhileTyping(true)
		]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SIconButton).Icon(FAppStyle::Get().GetBrush("GenericCommands.Delete"))
            .OnClicked_Lambda([this] {
                OutPutLogMarshaller->Clear();
                MessagesTextBox->Refresh();
                return FReply::Handled();
            })
        ]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
			.ToolTipText(FText::FromString("Add an output log filter."))
			.OnGetMenuContent_Lambda([this] { return MakeAddFilterMenu(); })
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 0, 0, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Filters"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];

		Filter = MakeShared<FOutputLogFilter>();
		OutPutLogMarshaller = MakeShared<FOutputLogMarshaller>(Filter.Get());

		SLogWidget::Construct(
			SLogWidget::FArguments()
			.Marshaller(OutPutLogMarshaller)
			.ExtraContentArea()
			[
				FilterBox.ToSharedRef()
			]
		);

		GLog->AddOutputDevice(this);
	}


	void SOutputLog::RequestForceScroll()
	{
		MessagesTextBox->ScrollTo(ETextLocation::EndOfDocument);
		bIsUserScrolled = false;
	}

	void SOutputLog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		auto OutputLogMarshaller = StaticCastSharedPtr<FOutputLogMarshaller>(Marshaller);
		if (OutputLogMarshaller->SubmitPendingMessages())
		{
			// Don't scroll to the bottom automatically when the user is scrolling the view or has scrolled it away from the bottom.
			if (!bIsUserScrolled)
			{
				RequestForceScroll();
			}
		}

		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}

	void SOutputLog::OnFilterTextChanged(const FText& InFilterText)
	{
		Filter->FilterString = InFilterText.ToString();
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	void SOutputLog::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
	{
		OnFilterTextChanged(InFilterText);
	}

	TSharedRef<SWidget> SOutputLog::MakeAddFilterMenu()
	{
		FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);

		MenuBuilder.BeginSection("OutputLogVerbosityEntries", FText::FromString("Verbosity"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Messages"),
				FText::FromString("Filter the Output Log to show messages"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityLogs_Execute),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityLogs_IsChecked)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			MenuBuilder.AddMenuEntry(
				FText::FromString("Warnings"),
				FText::FromString("Filter the Output Log to show warnings"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityWarnings_Execute),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityWarnings_IsChecked)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			MenuBuilder.AddMenuEntry(
				FText::FromString("Errors"),
				FText::FromString("Filter the Output Log to show errors"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityErrors_Execute),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityErrors_IsChecked)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("OutputLogMiscEntries", FText::FromString("Categories"));
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Project"),
				FText::FromString("Filter the Output Log to show SH_LOG"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::ShLog_Execute),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateSP(this, &SOutputLog::ShLog_IsChecked)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			MenuBuilder.AddMenuEntry(
				FText::FromString("UE"),
				FText::FromString("Filter the Output Log to show UE_LOG"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::UeLog_Execute),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateSP(this, &SOutputLog::UeLog_IsChecked)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}

		return MenuBuilder.MakeWidget();
		
	}

	void SOutputLog::VerbosityLogs_Execute()
	{
		Filter->bShowLogs ^= 1;
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	bool SOutputLog::VerbosityLogs_IsChecked() const
	{
		return Filter->bShowLogs;
	}

	void SOutputLog::VerbosityWarnings_Execute()
	{
		Filter->bShowWarnings ^= 1;
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	bool SOutputLog::VerbosityWarnings_IsChecked() const
	{
		return Filter->bShowWarnings;
	}

	void SOutputLog::VerbosityErrors_Execute()
	{
		Filter->bShowErrors ^= 1;
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	bool SOutputLog::VerbosityErrors_IsChecked() const
	{
		return Filter->bShowErrors;
	}

	void SOutputLog::UeLog_Execute()
	{
		Filter->bShowUeLog ^= 1;
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	bool SOutputLog::UeLog_IsChecked() const
	{
		return Filter->bShowUeLog;
	}

	void SOutputLog::ShLog_Execute()
	{
		Filter->bShowShLog ^= 1;
		Marshaller->MakeDirty();
		MessagesTextBox->Refresh();
	}

	bool SOutputLog::ShLog_IsChecked() const
	{
		return Filter->bShowShLog;
	}

	static const FName NAME_StyleLogError(TEXT("Log.Error"));
	static const FName NAME_StyleLogWarning(TEXT("Log.Warning"));
	static const FName NAME_StyleLogNormal(TEXT("Log.Normal"));

	void SOutputLog::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
	{
		auto OutputLogMarshaller = StaticCastSharedPtr<FOutputLogMarshaller>(Marshaller);

		FName Style;
		if (Verbosity == ELogVerbosity::Error)
		{
			Style = NAME_StyleLogError;
		}
		else if (Verbosity == ELogVerbosity::Warning)
		{
			Style = NAME_StyleLogWarning;
		}
		else
		{
			Style = NAME_StyleLogNormal;
		}
		
		// handle multiline strings by breaking them apart by line
		TArray<FTextRange> LineRanges;
		FString CurrentLogDump = V;
		FTextRange::CalculateLineRangesFromString(CurrentLogDump, LineRanges);

		bool bIsFirstLineInMessage = true;
		for (const FTextRange& LineRange : LineRanges)
		{
			if (!LineRange.IsEmpty())
			{
				FString Line = CurrentLogDump.Mid(LineRange.BeginIndex, LineRange.Len());
				Line = Line.ConvertTabsToSpaces(4);

				// Hard-wrap lines to avoid them being too long
				static const int32 HardWrapLen = 600;
				for (int32 CurrentStartIndex = 0; CurrentStartIndex < Line.Len();)
				{
					int32 HardWrapLineLen = 0;
					if (bIsFirstLineInMessage)
					{
						const FString MessagePrefix = FOutputDeviceHelper::FormatLogLine(Verbosity, Category, nullptr);

						HardWrapLineLen = FMath::Min(HardWrapLen - MessagePrefix.Len(), Line.Len() - CurrentStartIndex);
						const FString HardWrapLine = Line.Mid(CurrentStartIndex, HardWrapLineLen);

						OutputLogMarshaller->AddMessage({MakeShared<FString>(MessagePrefix + HardWrapLine), Verbosity, Category, Style});
					}
					else
					{
						HardWrapLineLen = FMath::Min(HardWrapLen, Line.Len() - CurrentStartIndex);
						FString HardWrapLine = Line.Mid(CurrentStartIndex, HardWrapLineLen);

						OutputLogMarshaller->AddMessage({MakeShared<FString>(MoveTemp(HardWrapLine)), Verbosity, Category, Style});
					}

					bIsFirstLineInMessage = false;
					CurrentStartIndex += HardWrapLineLen;
				}
			}
		}
	}

	FOutputLogMarshaller::FOutputLogMarshaller(FOutputLogFilter* InFilter)
		:NextPendingMessageIndex(0), Filter(InFilter), TextLayout(nullptr)
	{

	}

    void FOutputLogMarshaller::Clear()
    {
        Messages.Empty();
        MakeDirty();
    }

	void FOutputLogMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;
		NextPendingMessageIndex = 0;
	}

	void FOutputLogMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	void FOutputLogMarshaller::AddMessage(FOutputLogMessage InMessage)
	{
		Messages.Add(MoveTemp(InMessage));
	}

	bool FOutputLogMarshaller::SubmitPendingMessages()
	{
		if (Messages.IsValidIndex(NextPendingMessageIndex))
		{
			const int32 CurrentMessagesCount = Messages.Num();
			AppendPendingMessagesToTextLayout();
			NextPendingMessageIndex = CurrentMessagesCount;
			return true;
		}
		return false;
	}

	void FOutputLogMarshaller::AppendPendingMessagesToTextLayout()
	{
		const int32 CurrentMessagesCount = Messages.Num();
		const int32 NumPendingMessages = CurrentMessagesCount - NextPendingMessageIndex;

		if (NumPendingMessages == 0)
		{
			return;
		}

		TArray<FTextLayout::FNewLineData> LinesToAdd;
		LinesToAdd.Reserve(NumPendingMessages);
		int32 NumAddedMessages = 0;

		for (int32 MessageIndex = NextPendingMessageIndex; MessageIndex < CurrentMessagesCount; ++MessageIndex)
		{
			const FOutputLogMessage& Message = Messages[MessageIndex];
			const int32 LineIndex = TextLayout->GetLineModels().Num() + NumAddedMessages;

			Filter->AddAvailableLogCategory(Message.Category);
			if (!Filter->IsMessageAllowed(Message))
			{
				continue;
			}

			++NumAddedMessages;

			const FTextBlockStyle& MessageTextStyle = FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>(Message.Style);

			TSharedRef<FString> LineText = Message.Message.ToSharedRef();

			TArray<TSharedRef<IRun>> Runs;
			Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, MessageTextStyle));
			LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
		}
		
		TextLayout->AddLines(LinesToAdd);
	}

	int32 FOutputLogMarshaller::GetNumMessages() const
	{
		const int32 NumPendingMessages = Messages.Num() - NextPendingMessageIndex;
		return Messages.Num() - NumPendingMessages;
	}

	void FOutputLogFilter::AddAvailableLogCategory(FName LogCategory)
	{
		// Use an insert-sort to keep AvailableLogCategories alphabetically sorted
		int32 InsertIndex = 0;
		for (InsertIndex = AvailableLogCategories.Num() - 1; InsertIndex >= 0; --InsertIndex)
		{
			FName CheckCategory = AvailableLogCategories[InsertIndex];
			// No duplicates
			if (CheckCategory == LogCategory)
			{
				return;
			}
			else if (CheckCategory.Compare(LogCategory) < 0)
			{
				break;
			}
		}
		AvailableLogCategories.Insert(MoveTemp(LogCategory), InsertIndex + 1);
	}

	bool FOutputLogFilter::IsMessageAllowed(const FOutputLogMessage& Message)
	{
		// Filter Verbosity
		{
			if (Message.Verbosity == ELogVerbosity::Error && !bShowErrors)
			{
				return false;
			}

			if (Message.Verbosity == ELogVerbosity::Warning && !bShowWarnings)
			{
				return false;
			}

			if (Message.Verbosity != ELogVerbosity::Error && Message.Verbosity != ELogVerbosity::Warning && !bShowLogs)
			{
				return false;
			}
		}

		
		{
			if (FilterString.Len() > 0 && !Message.Message->Contains(FilterString)) {
				return false;
			}
		}

		{
			bool bIsShLog = GProjectCategoryNames.Contains(Message.Category);
			bool bIsUeLog = !bIsShLog;
			if (!bShowUeLog && bIsUeLog) {
				return false;
			}

			if (!bShowShLog && bIsShLog) {
				return false;
			}
		}

		return true;
	}

}
