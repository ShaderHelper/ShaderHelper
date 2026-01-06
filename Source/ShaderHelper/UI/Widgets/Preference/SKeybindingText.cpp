#include "CommonHeader.h"
#include "SKeybindingText.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"

using namespace FW;

namespace SH
{
	void SKeybindingText::Construct(const FArguments& InArgs)
	{
		Command = InArgs._Command;
		if (Command)
		{
			bIsReadOnly = true;
			SInlineEditableTextBlock::Construct(SInlineEditableTextBlock::FArguments());
			SetText(TAttribute<FText>::CreateLambda([this] { return Command->GetActiveChord(EMultipleKeyBindingIndex::Primary)->GetInputText(false); }));
			TextBox->SetOnKeyDownHandler(FOnKeyDown::CreateLambda([this](const FGeometry&, const FKeyEvent& InKeyEvent) {
				const FKey Key = InKeyEvent.GetKey();
				EditingInputChord.bCtrl = InKeyEvent.IsControlDown();
				EditingInputChord.bAlt = InKeyEvent.IsAltDown();
				EditingInputChord.bShift = InKeyEvent.IsShiftDown();
				EditingInputChord.bCmd = InKeyEvent.IsCommandDown();

				if (!EKeys::IsModifierKey(Key))
				{
					EditingInputChord.Key = Key;
				}

				SetText(EditingInputChord.GetInputText(false));
				return FReply::Handled();
			}));
			TextBox->SetOnKeyCharHandler(FOnKeyChar::CreateLambda([this](const FGeometry&, const FCharacterEvent&) { return FReply::Handled(); }));
			OnEnterEditingMode = FSimpleDelegate::CreateLambda([this] {
				EditingInputChord = FInputChord(EKeys::Invalid, EModifierKey::None);

				SetText(FText::GetEmpty());
			});
			OnExitEditingMode = FSimpleDelegate::CreateLambda([this] {

				TSharedPtr<FUICommandInfo> ConflictingCommand;

				if (EditingInputChord != EKeys::Invalid)
				{
					TArray<TSharedPtr<FUICommandInfo>> CommandsInContext;
					FInputBindingManager::Get().GetCommandInfosFromContext(Command->GetBindingContext(), CommandsInContext);
					for (const auto& OtherCommand : CommandsInContext)
					{
						if (OtherCommand != Command &&
							*OtherCommand->GetActiveChord(EMultipleKeyBindingIndex::Primary) == EditingInputChord)
						{
							ConflictingCommand = OtherCommand;
							break;
						}
					}
				}

				if (!ConflictingCommand)
				{
					Command->SetActiveChord(EditingInputChord, EMultipleKeyBindingIndex::Primary);
					FInputBindingManager::Get().SaveInputBindings();
					GConfig->Flush(false, GEditorPerProjectIni);
				}
				else
				{
					FText Message = FText::Format(LOCALIZATION("BindingConflict"), EditingInputChord.GetInputText(false), ConflictingCommand->GetLabel());
					MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, FSlateApplication::Get().FindWidgetWindow(AsShared()), Message);
				}

				SetText(TAttribute<FText>::CreateLambda([this] { return Command->GetActiveChord(EMultipleKeyBindingIndex::Primary)->GetInputText(false);  }));
			});
		}
	}
}