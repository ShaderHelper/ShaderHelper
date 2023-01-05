#include "CommonHeader.h"
#include "SShaderHelperWindow.h"
#include "App/ShaderHelperApp.h"

#define LOCTEXT_NAMESPACE "ShaderHelperWindow"

namespace SH {

	TSharedRef<SDockTab> SShaderHelperWindow::SpawnWindowTab(const FSpawnTabArgs& Args)
	{
		const FString& TabName = Args.GetTabId().ToString();
		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);
		if (TabName == "PreviewTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(LOCTEXT("PreviewTabLabel", "Preview"));
		}
		else if (TabName == "PropetyTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(LOCTEXT("PropetyTabLabel", "Propety"));
		}
		else if (TabName == "CodeTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(LOCTEXT("CodeTabLabel", "Code"));

		}
		else if (TabName == "ResourceTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(LOCTEXT("ResourceTabLabel", "Resource"));
		}
		else {
			ensure(false);
		}
		return SpawnedTab;
	}

	void SShaderHelperWindow::Construct(const FArguments& InArgs)
	{

		TSharedRef<SDockTab> NewTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
		TabManager = FGlobalTabmanager::Get()->NewTabManager(NewTab);
		TabManager->RegisterTabSpawner("PreviewTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(LOCTEXT("PreviewTitle", "Preview"));
		TabManager->RegisterTabSpawner("PropetyTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(LOCTEXT("PropetyTitle", "Propety"));
		TabManager->RegisterTabSpawner("ResourceTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(LOCTEXT("ResourceTitle", "Resource"));
		TabManager->RegisterTabSpawner("CodeTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(LOCTEXT("CodeTitle", "Code"));

		TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ShaderHelperLayout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.4f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->AddTab("PreviewTab", ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab("ResourceTab", ETabState::OpenedTab)
				)
				
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.15f)
				->AddTab("PropetyTab", ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.45f)
				->AddTab("CodeTab", ETabState::OpenedTab)
			)
		);

		SWindow::Construct(SWindow::FArguments()
			.Title(LOCTEXT("WindowTitle","ShaderHelper"))
			.ClientSize(ShaderHelperApp::DefaultClientSize)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuBar()
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					TabManager->RestoreFrom(Layout, TSharedPtr<SWindow>()).ToSharedRef()
				]
			]
		);
			
	}

	TSharedRef<SWidget> SShaderHelperWindow::CreateMenuBar()
	{
		FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
		MenuBarBuilder.AddPullDownMenu(
			LOCTEXT("FileMenu", "File"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("File"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCTEXT("ConfigMenu", "Config"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("Config"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCTEXT("WindowMenu", "Window"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("Window"))
		);


		return MenuBarBuilder.MakeWidget();
	}

	void SShaderHelperWindow::FillMenu(FMenuBuilder& MenuBuilder, FString MenuName)
	{
		if (MenuName == "File") {
			MenuBuilder.AddMenuEntry(LOCTEXT("Title1", "TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Config") {
			MenuBuilder.AddMenuEntry(LOCTEXT("Title1", "TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Window") {
			MenuBuilder.AddMenuEntry(LOCTEXT("Title1", "TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
	}

}