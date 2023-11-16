#include "CommonHeader.h"
#include "SShaderHelperWindow.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "SShaderHelperPropertyView.h"

namespace SH 
{

	TSharedRef<SDockTab> SShaderHelperWindow::SpawnWindowTab(const FSpawnTabArgs& Args)
	{
		const FString& TabName = Args.GetTabId().ToString();
		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);
		if (TabName == "PreviewTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromString("Preview"))
				[
					SAssignNew(Viewport,SViewport)
				];
		}
		else if (TabName == "PropetyTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromString("Propety"))
				[
					SNew(SShaderHelperPropertyView)
					.Renderer(Renderer)
					.ShaderEditor_Lambda([this] {
						return ShaderEditor.Get();
					})
				];
		}
		else if (TabName == "CodeTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromString("Code"))
				[
					SAssignNew(ShaderEditor, SShaderEditorBox)
						.Text(FText::FromString(ShRenderer::DefaultPixelShaderText))
						.Renderer(Renderer)
				];

		}
		else if (TabName == "ResourceTab") {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromString("Resource"));
		}
		else {
			ensure(false);
		}
		return SpawnedTab;
	}

	void SShaderHelperWindow::Construct(const FArguments& InArgs)
	{
		Renderer = InArgs._Renderer;
		WindowSize = InArgs._WindowSize;

		TSharedRef<SDockTab> NewTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
		TabManager = FGlobalTabmanager::Get()->NewTabManager(NewTab);
		TabManager->RegisterTabSpawner("PreviewTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(FText::FromString("Preview"));
		TabManager->RegisterTabSpawner("PropetyTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(FText::FromString("Propety"));
		TabManager->RegisterTabSpawner("ResourceTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(FText::FromString("Resource"));
		TabManager->RegisterTabSpawner("CodeTab", FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab))
			.SetDisplayName(FText::FromString("Code"));

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
				->SetSizeCoefficient(0.18f)
				->AddTab("PropetyTab", ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.42f)
				->AddTab("CodeTab", ETabState::OpenedTab)
			)
		);

		SWindow::Construct(SWindow::FArguments()
			.Title(FText::FromString("ShaderHelper"))
			.ClientSize(WindowSize)
			.MinWidth(500)
			.MinHeight(400)
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
			FText::FromString("File"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("File"))
		);
		MenuBarBuilder.AddPullDownMenu(
			FText::FromString("Config"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("Config"))
		);
		MenuBarBuilder.AddPullDownMenu(
			FText::FromString("Window"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &SShaderHelperWindow::FillMenu, FString("Window"))
		);
        
        TSharedRef<SWidget> MenuWidget = MenuBarBuilder.MakeWidget();
        //Add native menu bar for the window on mac.
        TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuWidget);
        TabManager->UpdateMainMenu(nullptr, true);
        
        return MenuWidget;
	}

	void SShaderHelperWindow::FillMenu(FMenuBuilder& MenuBuilder, FString MenuName)
	{
		if (MenuName == "File") {
			MenuBuilder.AddMenuEntry(FText::FromString("TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Config") {
			MenuBuilder.AddMenuEntry(FText::FromString("TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Window") {
			MenuBuilder.AddMenuEntry(FText::FromString("TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
	}

}
