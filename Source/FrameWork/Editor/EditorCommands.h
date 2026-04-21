#pragma once
#include "UI/Styles/FAppCommonStyle.h"

#include <Framework/Commands/Commands.h>

namespace FW
{
	class EditorCommands : public TCommands<EditorCommands>
	{
	public:

		EditorCommands()
			: TCommands<EditorCommands>(TEXT("EditorCommands"), {}, NAME_None, FAppCommonStyle::Get().GetStyleSetName())
		{
		}

		virtual ~EditorCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE ""
			UI_COMMAND_SH(Undo, LOCALIZATION("Undo"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Z));
			UI_COMMAND_SH(Redo, LOCALIZATION("Redo"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Y));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Undo;
		TSharedPtr<FUICommandInfo> Redo;
	};
}
