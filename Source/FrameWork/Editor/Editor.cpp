#include "CommonHeader.h"
#include "Editor.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "Common/Path/PathHelper.h"
#include "GraphEditorCommands.h"
#include "AssetViewCommands.h"

namespace FW 
{
	FString EditorConfigPath()
	{
		return PathHelper::SavedConfigDir() / TEXT("Editor.ini");
	}

	FString EditorKeyBindingConfigPath()
	{
		return PathHelper::SavedConfigDir() / TEXT("EditorKeybinding.ini");
	}

	Editor::Editor()
	{
		if (IFileManager::Get().FileExists(*EditorConfigPath()))
		{
			EditorConfig->Read(EditorConfigPath());

			FString Lang;
			EditorConfig->GetString(TEXT("Common"), TEXT("Language"), Lang);
			if (auto ConfigLang = magic_enum::enum_cast<SupportedLanguage>(TCHAR_TO_ANSI(*Lang), magic_enum::case_insensitive))
			{
				CurLanguage = ConfigLang.value();
			}
		}

		SetLanguage(CurLanguage);

		GraphEditorCommands::Register();
		AssetViewCommands::Register();
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
		EditorConfig->Write(EditorConfigPath());
	}

	void Editor::SaveEditorConfig()
	{
		EditorConfig->Write(EditorConfigPath());
	}

}
