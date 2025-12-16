#include "CommonHeader.h"
#include "Editor.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "Common/Path/PathHelper.h"
#include "GraphEditorCommands.h"
#include "AssetViewCommands.h"

#include <Framework/Commands/GenericCommands.h>

STEAL_PRIVATE_MEMBER(FUICommandInfo, FText, Label)
STEAL_PRIVATE_MEMBER(FUICommandInfo, FText, Description)

CALL_PRIVATE_FUNCTION(USlateThemeManager_LoadThemesFromDirectory, USlateThemeManager, LoadThemesFromDirectory, , void, const FString&)
CALL_PRIVATE_FUNCTION(USlateThemeManager_GetMutableCurrentTheme, USlateThemeManager, GetMutableCurrentTheme, , FStyleTheme&)

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
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User1, { 1.0f, 0.4f, 0.7f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User2, { 0.5f, 1.0f, 0.66f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User3, { 0.2f, 0.9f, 0.2f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User4, { 0.58f, 0.43f, 0.82f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User5, { 0.1f, 0.55f, 1.0f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User6, { 0.35f, 0.35f, 0.35f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User7, { 1.0f, 0.8f, 0.66f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User8, { 0.39f, 0.72f, 1.0f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User9, { 1.0f, 0.93f, 0.54f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User10, { 0.0f, 0.81f, 0.81f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User11, { 0.93f, 0.51f, 0.38f, 1.0f });
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User12, FLinearColor::White);
		USlateThemeManager::Get().SetDefaultColor(EStyleColor::User13, FLinearColor::White);
		CallPrivate_USlateThemeManager_LoadThemesFromDirectory(USlateThemeManager::Get(), PathHelper::ThemeDir());

		if (IFileManager::Get().FileExists(*EditorConfigPath()))
		{
			EditorConfig->Read(EditorConfigPath());

			FString Lang;
			EditorConfig->GetString(TEXT("Common"), TEXT("Language"), Lang);
			if (auto ConfigLang = magic_enum::enum_cast<SupportedLanguage>(TCHAR_TO_ANSI(*Lang), magic_enum::case_insensitive))
			{
				CurLanguage = ConfigLang.value();
			}

			FString ThemeId;
			EditorConfig->GetString(TEXT("Common"), TEXT("Theme"), ThemeId);
			if (!ThemeId.IsEmpty())
			{
				USlateThemeManager::Get().ApplyTheme(FGuid{ ThemeId });
			}
		}

		SetTheme(USlateThemeManager::Get().GetCurrentTheme().Id);
		SetLanguage(CurLanguage);
		ResetGenericCommandsLabelAndTip();

		GraphEditorCommands::Register();
		AssetViewCommands::Register();
	}

	Editor::~Editor()
	{
	
	}

	const FStyleTheme& Editor::GetTheme(const FGuid& InId)
	{
		return *USlateThemeManager::Get().GetThemes().FindByPredicate([InId](auto&& InItem) { return InId == InItem.Id; });
	}

	FStyleTheme& Editor::GetMutableCurrentTheme()
	{
		return CallPrivate_USlateThemeManager_GetMutableCurrentTheme(USlateThemeManager::Get());
	}

	void Editor::SetTheme(const FGuid& InId)
	{
		USlateThemeManager::Get().ApplyTheme(InId);
		EditorConfig->SetString(TEXT("Common"), TEXT("Theme"), *InId.ToString());
		EditorConfig->Write(EditorConfigPath());
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
