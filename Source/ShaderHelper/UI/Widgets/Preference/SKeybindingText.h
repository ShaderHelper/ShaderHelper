#pragma once
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace SH
{
	class SKeybindingText : public SInlineEditableTextBlock
	{
		SLATE_BEGIN_ARGS(SKeybindingText) {}
			SLATE_ARGUMENT(TSharedPtr<FUICommandInfo>, Command)
		SLATE_END_ARGS()
	public:
		void Construct(const FArguments& InArgs)
		{
			Command = InArgs._Command;
			if (Command)
			{
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
						ExitEditingMode();
					}
					else
					{
						TextBox->SetText(EditingInputChord.GetInputText(false));
					}
					return FReply::Handled();
				}));
				OnEnterEditingMode = FSimpleDelegate::CreateLambda([this]{
					EditingInputChord = FInputChord(EKeys::Invalid, EModifierKey::None);

					SetText(FText::GetEmpty());
				});
				OnExitEditingMode = FSimpleDelegate::CreateLambda([this] {
					Command->SetActiveChord(EditingInputChord, EMultipleKeyBindingIndex::Primary);
					FInputBindingManager::Get().SaveInputBindings();
					GConfig->Flush(false, GEditorPerProjectIni);

					SetText(TAttribute<FText>::CreateLambda([this] { return Command->GetActiveChord(EMultipleKeyBindingIndex::Primary)->GetInputText(false);  }));
				});
			}
		}

	private:
		TSharedPtr<FUICommandInfo> Command;
		FInputChord EditingInputChord;
	};
}