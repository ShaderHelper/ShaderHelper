#pragma once
#include <Framework/Commands/Commands.h>
#include "UI/Styles/FAppCommonStyle.h"

namespace FRAMEWORK
{
	class CommonCommands : public TCommands<CommonCommands>
	{
	public:

		CommonCommands()
			: TCommands<CommonCommands>(TEXT("CommonCommands"), NSLOCTEXT("CommonCommands", "Common Commands", "Common Commands"), NAME_None, FAppCommonStyle::Get().GetStyleSetName())
		{
		}

		virtual ~CommonCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE "CommonCommands"
			UI_COMMAND(Save, "Save", "Save the current item", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));

#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Save;
	};
}