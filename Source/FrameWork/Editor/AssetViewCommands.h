#pragma once
#include "UI/Styles/FAppCommonStyle.h"

#include <Framework/Commands/Commands.h>

namespace FW
{
	class AssetViewCommands : public TCommands<AssetViewCommands>
	{
	public:

		AssetViewCommands()
			: TCommands<AssetViewCommands>(TEXT("AssetViewCommands"), {}, NAME_None, FAppCommonStyle::Get().GetStyleSetName())
		{
		}

		virtual ~AssetViewCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE ""
			UI_COMMAND_SH(Delete, LOCALIZATION("Delete"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
			UI_COMMAND_SH(Rename, LOCALIZATION("Rename"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F2));
			UI_COMMAND_SH(Save, LOCALIZATION("Save"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Delete;
		TSharedPtr<FUICommandInfo> Rename;
		TSharedPtr<FUICommandInfo> Save;
	};
}
