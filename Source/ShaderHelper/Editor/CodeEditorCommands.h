#pragma once
#include <Framework/Commands/Commands.h>
#include "UI/Styles/FShaderHelperStyle.h"

namespace SH
{
	class CodeEditorCommands : public TCommands<CodeEditorCommands>
	{
	public:

		CodeEditorCommands()
			: TCommands<CodeEditorCommands>(TEXT("CodeEditorCommands"), {}, NAME_None, FShaderHelperStyle::Get().GetStyleSetName())
		{
		}

		virtual ~CodeEditorCommands()
		{
		}

		virtual void RegisterCommands() override
		{
#define LOCTEXT_NAMESPACE ""
			UI_COMMAND_SH(Cut, LOCALIZATION("Cut"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::X));
			UI_COMMAND_SH(Copy, LOCALIZATION("Copy"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::C));
			UI_COMMAND_SH(Paste, LOCALIZATION("Paste"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::V));
			UI_COMMAND_SH(Undo, LOCALIZATION("Undo"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Z));
			UI_COMMAND_SH(Redo, LOCALIZATION("Redo"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Z));
			UI_COMMAND_SH(DeleteLeft, LOCALIZATION("DeleteLeft"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::BackSpace));
			UI_COMMAND_SH(DeleteRight, LOCALIZATION("DeleteRight"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::Delete));
			UI_COMMAND_SH(SelectAll, LOCALIZATION("SelectAll"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::A));
			UI_COMMAND_SH(Save, LOCALIZATION("Save"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));
			UI_COMMAND_SH(Debug, LOCALIZATION("Debug"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F5));
			UI_COMMAND_SH(Continue, LOCALIZATION("Continue"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F9));
			UI_COMMAND_SH(StepInto, LOCALIZATION("StepInto"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F11));
			UI_COMMAND_SH(StepOver, LOCALIZATION("StepOver"), FText::GetEmpty(), EUserInterfaceActionType::Button, FInputChord(EKeys::F10));
			UI_COMMAND_SH(ToggleComment, LOCALIZATION("ToggleComment"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Slash));
			UI_COMMAND_SH(GoToBeginningOfLine, LOCALIZATION("GoToBeginningOfLine"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::Home));
			UI_COMMAND_SH(GoToEndOfLine, LOCALIZATION("GoToEndOfLine"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EKeys::End));
			UI_COMMAND_SH(CursorSelectLeft, LOCALIZATION("CursorSelectLeft"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Shift, EKeys::Left));
			UI_COMMAND_SH(CursorSelectRight, LOCALIZATION("CursorSelectRight"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Shift, EKeys::Right));
			UI_COMMAND_SH(CursorSelectLineStart, LOCALIZATION("CursorSelectLineStart"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Shift, EKeys::Home));
			UI_COMMAND_SH(CursorSelectLineEnd, LOCALIZATION("CursorSelectLineEnd"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Shift, EKeys::End));
			UI_COMMAND_SH(CursorTokenSelectLeft, LOCALIZATION("CursorTokenSelectLeft"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Left));
			UI_COMMAND_SH(CursorTokenSelectRight, LOCALIZATION("CursorTokenSelectRight"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Right));
			UI_COMMAND_SH(CursorTokenLeft, LOCALIZATION("CursorTokenLeft"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Left));
			UI_COMMAND_SH(CursorTokenRight, LOCALIZATION("CursorTokenRight"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Right));
			UI_COMMAND_SH(ScrollUp, LOCALIZATION("ScrollUp"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Up));
			UI_COMMAND_SH(ScrollDown, LOCALIZATION("ScrollDown"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Down));
			UI_COMMAND_SH(JumpTop, LOCALIZATION("JumpTop"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::Home));
			UI_COMMAND_SH(JumpBottom, LOCALIZATION("JumpBottom"), FText::GetEmpty(), EUserInterfaceActionType::None, FInputChord(EModifierKey::Control, EKeys::End));
#undef LOCTEXT_NAMESPACE
		}

		TSharedPtr<FUICommandInfo> Cut;
		TSharedPtr<FUICommandInfo> Copy;
		TSharedPtr<FUICommandInfo> Paste;
		TSharedPtr<FUICommandInfo> Undo;
		TSharedPtr<FUICommandInfo> Redo;
		TSharedPtr<FUICommandInfo> DeleteLeft;
		TSharedPtr<FUICommandInfo> DeleteRight;
		TSharedPtr<FUICommandInfo> SelectAll;
		TSharedPtr<FUICommandInfo> Save;

		TSharedPtr<FUICommandInfo> Debug;
		TSharedPtr<FUICommandInfo> Continue;
		TSharedPtr<FUICommandInfo> StepInto;
		TSharedPtr<FUICommandInfo> StepOver;
		TSharedPtr<FUICommandInfo> ToggleComment;
		TSharedPtr<FUICommandInfo> GoToBeginningOfLine;
		TSharedPtr<FUICommandInfo> GoToEndOfLine;
		TSharedPtr<FUICommandInfo> CursorSelectLeft;
		TSharedPtr<FUICommandInfo> CursorSelectRight;
		TSharedPtr<FUICommandInfo> CursorSelectLineStart;
		TSharedPtr<FUICommandInfo> CursorSelectLineEnd;
		TSharedPtr<FUICommandInfo> CursorTokenSelectLeft;
		TSharedPtr<FUICommandInfo> CursorTokenSelectRight;
		TSharedPtr<FUICommandInfo> CursorTokenLeft;
		TSharedPtr<FUICommandInfo> CursorTokenRight;
		TSharedPtr<FUICommandInfo> ScrollUp;
		TSharedPtr<FUICommandInfo> ScrollDown;
		TSharedPtr<FUICommandInfo> JumpTop;
		TSharedPtr<FUICommandInfo> JumpBottom;
	};
}
