#pragma once
#include <Framework/Commands/Commands.h>
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{
	class FRAMEWORK_API CommonCommands : public TCommands<CommonCommands>
	{
	public:

		CommonCommands()
			: TCommands<CommonCommands>(TEXT("CommonCommands"), {}, NAME_None, FAppCommonStyle::Get().GetStyleSetName())
		{
		}

		virtual ~CommonCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE "CommonCommands"
			UI_COMMAND(Save, "", "", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));
			UI_COMMAND(Continue, "", "", EUserInterfaceActionType::Button, FInputChord(EKeys::F5));
			UI_COMMAND(StepInto, "", "", EUserInterfaceActionType::Button, FInputChord(EKeys::F11));
			UI_COMMAND(StepOver, "", "", EUserInterfaceActionType::Button, FInputChord(EKeys::F10));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Save;
		TSharedPtr<FUICommandInfo> Continue;
		TSharedPtr<FUICommandInfo> StepInto;
		TSharedPtr<FUICommandInfo> StepOver;
	};
}
