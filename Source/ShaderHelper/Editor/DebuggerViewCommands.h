#pragma once
#include <Framework/Commands/Commands.h>
#include "UI/Styles/FShaderHelperStyle.h"

namespace SH
{
	class DebuggerViewCommands : public TCommands<DebuggerViewCommands>
	{
	public:

		DebuggerViewCommands()
			: TCommands<DebuggerViewCommands>(TEXT("DebuggerViewCommands"), {}, NAME_None, FShaderHelperStyle::Get().GetStyleSetName())
		{
		}

		virtual ~DebuggerViewCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE ""
			UI_COMMAND_SH(Delete, LOCALIZATION("Delete"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Delete;
	};
}
