#pragma once
#include "UI/Styles/FShaderHelperStyle.h"

#include <Framework/Commands/Commands.h>

namespace SH
{
	class SceneViewCommands : public TCommands<SceneViewCommands>
	{
	public:

		SceneViewCommands()
			: TCommands<SceneViewCommands>(TEXT("SceneViewCommands"), {}, NAME_None, FShaderHelperStyle::Get().GetStyleSetName())
		{
		}

		virtual ~SceneViewCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE ""
			UI_COMMAND_SH(GizmoMove, LOCALIZATION("Move"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::W));
			UI_COMMAND_SH(GizmoRotate, LOCALIZATION("Rotate"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::E));
			UI_COMMAND_SH(GizmoScale, LOCALIZATION("Scale"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::R));
			UI_COMMAND_SH(DeleteObject, LOCALIZATION("Delete"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
			UI_COMMAND_SH(RenameObject, LOCALIZATION("Rename"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F2));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> GizmoMove;
		TSharedPtr<FUICommandInfo> GizmoRotate;
		TSharedPtr<FUICommandInfo> GizmoScale;
		TSharedPtr<FUICommandInfo> DeleteObject;
		TSharedPtr<FUICommandInfo> RenameObject;
	};
}
