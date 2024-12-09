#include "CommonHeader.h"
#include "Editor.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "Common/Path/PathHelper.h"
#include "magic_enum.hpp"
#include <Framework/Commands/GenericCommands.h>

STEAL_PRIVATE_MEMBER(FUICommandInfo, FText, Label)
STEAL_PRIVATE_MEMBER(FUICommandInfo, FText, Description)

namespace FRAMEWORK 
{
	FString BaseEditorConfig()
	{
		return PathHelper::SavedConfigDir() / TEXT("BaseEditor.ini");
	}

	static void ResetGenericCommandsLabelAndTip()
	{
		//Hook, reset GenericCommand label for Localization
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Cut) = LOCALIZATION("Cut");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Copy) = LOCALIZATION("Copy");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Paste) = LOCALIZATION("Paste");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Undo) = LOCALIZATION("Undo");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Delete) = LOCALIZATION("Delete");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().Rename) = LOCALIZATION("Rename");
		GetPrivate_FUICommandInfo_Label(*FGenericCommands::Get().SelectAll) = LOCALIZATION("SelectAll");

		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Cut) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Copy) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Paste) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Undo) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Delete) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().Rename) = FText::GetEmpty();
		GetPrivate_FUICommandInfo_Description(*FGenericCommands::Get().SelectAll) = FText::GetEmpty();
	}

	Editor::Editor()
	{
		if (IFileManager::Get().FileExists(*BaseEditorConfig()))
		{
			EditorConfig->Read(BaseEditorConfig());

			FString Lang;
			EditorConfig->GetString(TEXT("Common"), TEXT("Language"), Lang);
			if (auto ConfigLang = magic_enum::enum_cast<SupportedLanguage>(TCHAR_TO_ANSI(*Lang), magic_enum::case_insensitive))
			{
				CurLanguage = ConfigLang.value();
			}
		}

		SetLanguage(CurLanguage);
		ResetGenericCommandsLabelAndTip();
	}

	Editor::~Editor()
	{
	
	}

	void Editor::SetLanguage(SupportedLanguage InLanguage)
	{
		FStringTableRegistry::Get().UnregisterStringTable(TEXT("Localization"));
		FStringTableRegistry::Get().Internal_LocTableFromFile(TEXT("Localization"), TEXT("Localization"), 
			FString::Printf(TEXT("Localization/%s.csv"), ANSI_TO_TCHAR(magic_enum::enum_name(InLanguage).data())),
			PathHelper::ResourceDir());
		CurLanguage = InLanguage;

		EditorConfig->SetString(TEXT("Common"), TEXT("Language"), ANSI_TO_TCHAR(magic_enum::enum_name(CurLanguage).data()));
		EditorConfig->Write(BaseEditorConfig());
	}

}
