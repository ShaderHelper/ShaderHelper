#include "CommonHeader.h"
#include "SShaderHelperWindow.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Property/PropertyView/SShaderPassPropertyView.h"
#include "ProjectManager/ShProjectManager.h"
#include <Json/Serialization/JsonSerializer.h>
#include <Core/Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"

namespace SH 
{

	static const FString WindowLayoutConfigFileName = PathHelper::SavedConfigDir() / TEXT("WindowLayout.json");

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";
	const FName CodeTabId = "Code";
	const FName AssetTabId = "Asset";

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId
	};

	TSharedRef<SDockTab> SShaderHelperWindow::SpawnWindowTab(const FSpawnTabArgs& Args)
	{
		const FTabId& TabId = Args.GetTabId();
		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);

		if (TabId == PreviewTabId) {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromName(PreviewTabId))
				[
					Viewport.ToSharedRef()
				];
		}
		else if (TabId == PropretyTabId) {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromName(PropretyTabId))
				[
					SNew(SShaderPassPropertyView)
					.Renderer(Renderer)
					.ShaderEditor_Lambda([this] {
						return ShaderEditor.Get();
					})
				];
		}
		else if (TabId == CodeTabId) {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromName(CodeTabId))
				[
					SAssignNew(ShaderEditor, SShaderEditorBox)
						.Text(FText::FromString(ShRenderer::DefaultPixelShaderText))
						.Renderer(Renderer)
				];

		}
		else if (TabId == AssetTabId) {
			SAssignNew(SpawnedTab, SDockTab)
				.Label(FText::FromName(AssetTabId))
				[
					SNew(SAssetBrowser)
					.DirectoryShowed(TSingleton<ShProjectManager>::Get().GetActiveContentDirectory())
				];
		}
		else {
			ensure(false);
		}
		return SpawnedTab;
	}

	SShaderHelperWindow::~SShaderHelperWindow()
	{
		if (bSaveWindowLayout)
		{
			TabManager->SavePersistentLayout();
		}

		//Clean tab window
		TabManager->CloseAllAreas();
	}

	void SShaderHelperWindow::Construct(const FArguments& InArgs)
	{
		Renderer = InArgs._Renderer;
		WindowSize = InArgs._WindowSize;
		OnResetWindowLayout = InArgs._OnResetWindowLayout;

		TSharedRef<SDockTab> NewTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
		TabManager = FGlobalTabmanager::Get()->NewTabManager(NewTab);
		TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateRaw(this, &SShaderHelperWindow::SaveWindowLayout));

		for (const FName& TabId : TabIds)
		{
			TabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &SShaderHelperWindow::SpawnWindowTab));
		}

		DefaultTabLayout = FTabManager::NewLayout("ShaderHelperLayout")
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
					->SetSizeCoefficient(0.7f)
					->AddTab(PreviewTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(AssetTabId, ETabState::OpenedTab)
				)
				
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.18f)
				->AddTab(PropretyTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.42f)
				->AddTab(CodeTabId, ETabState::OpenedTab)
			)
		);

		SAssignNew(Viewport, SViewport);

		FVector2D UsedWindowPos = FVector2D::ZeroVector;
		EAutoCenter AutoCenterRule = EAutoCenter::PreferredWorkArea;
		FVector2D UsedWindowSize = WindowSize;
		TSharedRef<FTabManager::FLayout> UsedLayout = DefaultTabLayout.ToSharedRef();
		//Override default layout
		if (IFileManager::Get().FileExists(*WindowLayoutConfigFileName))
		{
			auto [LoadedPos, LoadedSize, LoadedLayout] = LoadWindowLayout(WindowLayoutConfigFileName);
			UsedLayout = LoadedLayout.ToSharedRef();
			UsedWindowSize = LoadedSize;
			UsedWindowPos = LoadedPos;
			AutoCenterRule = EAutoCenter::None;
		}
	

		SWindow::Construct(SWindow::FArguments()
			.Title(FText::FromString("ShaderHelper"))
			.ScreenPosition(UsedWindowPos)
			.AutoCenter(AutoCenterRule)
			.ClientSize(UsedWindowSize)
			.AdjustInitialSizeAndPositionForDPIScale(false)
			[
				SAssignNew(LayoutBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					CreateMenuBar()
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					TabManager->RestoreFrom(UsedLayout, TSharedPtr<SWindow>()).ToSharedRef()
				]
			]
		);

		SetCanTick(true);
			
	}

	void SShaderHelperWindow::ResetWindowLayout()
	{
		//Clean window layout cache to use default layout.
		IFileManager::Get().Delete(*WindowLayoutConfigFileName);
		bSaveWindowLayout = false;
		OnResetWindowLayout.ExecuteIfBound();
		TabManager->CloseAllAreas();
		FSlateApplication::Get().RequestDestroyWindow(SharedThis(this));
	}

	SShaderHelperWindow::WindowLayoutConfigInfo SShaderHelperWindow::LoadWindowLayout(const FString& InWindowLayoutConfigFileName)
	{
		TSharedPtr<FJsonObject> RootJsonObject;
		FString JsonContent;
		FFileHelper::LoadFileToString(JsonContent, *InWindowLayoutConfigFileName);

		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
		FJsonSerializer::Deserialize(Reader, RootJsonObject);

		TSharedPtr<FJsonObject> RootWindowSizeJsonObject = RootJsonObject->GetObjectField(TEXT("RootWindowSize"));
		FVector2D WindowSize = {
			RootWindowSizeJsonObject->GetNumberField(TEXT("Width")),
			RootWindowSizeJsonObject->GetNumberField(TEXT("Height"))
		};

		TSharedPtr<FJsonObject> RootWindowPosJsonObject = RootJsonObject->GetObjectField(TEXT("RootWindowPos"));
		FVector2D Pos = {
			RootWindowPosJsonObject->GetNumberField(TEXT("X")),
			RootWindowPosJsonObject->GetNumberField(TEXT("Y"))
		};

		TSharedPtr<FJsonObject> LayoutJsonObject = RootJsonObject->GetObjectField(TEXT("TabLayout"));
		TSharedPtr<FTabManager::FLayout> RelLayout = FTabManager::FLayout::NewFromJson(MoveTemp(LayoutJsonObject));

		return { Pos, WindowSize, RelLayout };
	}

	void SShaderHelperWindow::SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout)
	{
		TSharedRef<FJsonObject> RootJsonObject = MakeShared<FJsonObject>();

		TSharedRef<FJsonObject> RootWindowSizeJsonObject = MakeShared<FJsonObject>();
		Vector2D CurClientSize = GetClientSizeInScreen();
		RootWindowSizeJsonObject->SetNumberField(TEXT("Width"), CurClientSize.X);
		RootWindowSizeJsonObject->SetNumberField(TEXT("Height"), CurClientSize.Y);

		TSharedRef<FJsonObject> RootWindowPosJsonObject = MakeShared<FJsonObject>();
		Vector2D CurPos = GetPositionInScreen();
		RootWindowPosJsonObject->SetNumberField(TEXT("X"), CurPos.X);
		RootWindowPosJsonObject->SetNumberField(TEXT("Y"), CurPos.Y);

		TSharedRef<FJsonObject> LayoutJsonObject = InLayout->ToJson();
		RootJsonObject->SetObjectField(TEXT("RootWindowPos"), RootWindowPosJsonObject);
		RootJsonObject->SetObjectField(TEXT("RootWindowSize"), RootWindowSizeJsonObject);
		RootJsonObject->SetObjectField(TEXT("TabLayout"), LayoutJsonObject);

		FString NewJsonContents;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NewJsonContents);
		if (FJsonSerializer::Serialize(RootJsonObject, Writer))
		{
			FFileHelper::SaveStringToFile(NewJsonContents, *WindowLayoutConfigFileName);
		}
	}

	void SShaderHelperWindow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		if (IsWindowMinimized())
		{
			const TArray< TSharedRef<SWindow> > AllWindows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
			for (const TSharedRef<SWindow>& Window : AllWindows)
			{
				Window->Minimize();
			}
		}
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
		auto InvokeTabLambda = [this](const FName& TabId)
		{
			TSharedPtr<SDockTab> ExistingTab = TabManager->FindExistingLiveTab(TabId);
			if (ExistingTab)
			{
				ExistingTab->RequestCloseTab();
			}
			else
			{
				TabManager->TryInvokeTab(TabId);
			}
		};

		auto IsLiveTabLambda = [this](const FName& TabId)
		{
			return TabManager->FindExistingLiveTab(TabId).IsValid();
		};

		if (MenuName == "File") {
			MenuBuilder.AddMenuEntry(FText::FromString("TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Config") {
			MenuBuilder.AddMenuEntry(FText::FromString("TODO"), FText::GetEmpty(), FSlateIcon(), FUIAction());
		}
		else if (MenuName == "Window") {
			MenuBuilder.BeginSection("WindowTabControl", FText::FromString("Tab Control"));
			{
				for (const FName& TabId : TabIds)
				{
					MenuBuilder.AddMenuEntry(FText::FromName(TabId), FText::GetEmpty(),
						FSlateIcon{},
						FUIAction(
							FExecuteAction::CreateLambda(InvokeTabLambda, TabId),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda(IsLiveTabLambda, TabId)
						),
						NAME_None,
						EUserInterfaceActionType::ToggleButton
					);
				}
			}
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection("WindowLayout", FText::FromString("Layout"));
			{
				MenuBuilder.AddMenuEntry(FText::FromString("Reset Window Layout"), FText::GetEmpty(), 
					FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Refresh" }, 
					FUIAction(
						FExecuteAction::CreateRaw(this, &SShaderHelperWindow::ResetWindowLayout)
					));
			}
			MenuBuilder.EndSection();
		}
	}

}
