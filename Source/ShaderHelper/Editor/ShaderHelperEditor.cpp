#include "CommonHeader.h"
#include "ShaderHelperEditor.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Property/PropertyView/SShaderPropertyView.h"
#include "ProjectManager/ShProjectManager.h"
#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "magic_enum.hpp"
#include <Framework/Docking/SDockingTabStack.h>
#include "UI/Widgets/SShaderTab.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include <DesktopPlatformModule.h>

STEAL_PRIVATE_MEMBER(FTabManager, TArray<TSharedRef<FTabManager::FArea>>, CollapsedDockAreas)

using namespace FRAMEWORK;

namespace SH 
{
	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";

	const FName CodeTabId = "Code";
    const FName InitialInsertPointTabId = "CodeInsertPoint";

    const FName AssetTabId = "Asset";
	const FName GraphTabId = "Graph";

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId, GraphTabId
	};

    const TArray<FName> WindowMenuTabIds{
        PreviewTabId, PropretyTabId, AssetTabId, GraphTabId
    };

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
	{
		CurProject = &TSingleton<ShProjectManager>::Get().GetProject();

		ViewPort = MakeShared<PreviewViewPort>();
		ViewPort->OnViewportResize.AddRaw(this, &ShaderHelperEditor::OnViewportResize);

		InitEditorUI();
	}

	ShaderHelperEditor::~ShaderHelperEditor()
	{

	}

	void ShaderHelperEditor::InitEditorUI()
	{
		TabManagerTab = SNew(SDockTab);
		TabManager = FGlobalTabmanager::Get()->NewTabManager(TabManagerTab.ToSharedRef());
		TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateRaw(this, &ShaderHelperEditor::SaveWindowLayout));

		for (const FName& TabId : TabIds)
		{
			TabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnWindowTab));
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
						->SetSizeCoefficient(0.5f)
						->AddTab(PreviewTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(GraphTabId, ETabState::OpenedTab)
					)

				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.4f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(CodeTabId, ETabState::OpenedTab)
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
                     ->SetSizeCoefficient(0.15f)
                     ->AddTab(PropretyTabId, ETabState::OpenedTab)
                 )
			);

		FVector2D UsedWindowPos = FVector2D::ZeroVector;
		EAutoCenter AutoCenterRule = EAutoCenter::PreferredWorkArea;
		FVector2D UsedWindowSize = WindowSize;
		TSharedRef<FTabManager::FLayout> UsedLayout = DefaultTabLayout.ToSharedRef();
		//Override default layout
		if (IFileManager::Get().FileExists(*WindowLayoutFileName))
		{
			auto [LoadedPos, LoadedSize, LoadedLayout] = LoadWindowLayout(WindowLayoutFileName);
			UsedLayout = LoadedLayout.ToSharedRef();
			UsedWindowSize = LoadedSize;
			UsedWindowPos = LoadedPos;
			AutoCenterRule = EAutoCenter::None;
		}

		SAssignNew(Window, SWindow)
			.Title(TAttribute<FText>::CreateLambda([this] { 
				FString ProjectPath = TSingleton<ShProjectManager>::Get().GetProject().GetFilePath();
				return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + FString::Printf(TEXT(" [%s]"), *ProjectPath));
			}))
			.ScreenPosition(UsedWindowPos)
			.AutoCenter(AutoCenterRule)
			.ClientSize(UsedWindowSize)
			.AdjustInitialSizeAndPositionForDPIScale(false);

		TabManagerTab->AssignParentWidget(Window);

		FSlateApplication::Get().AddWindow(Window.ToSharedRef());
        
        auto MenuBarBuilder = CreateMenuBarBuilder();
        auto MenuBarWidget = MenuBarBuilder.MakeWidget();

		Window->SetContent(
			SAssignNew(WindowContentBox, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
                MenuBarWidget
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TabManager->RestoreFrom(UsedLayout, Window).ToSharedRef()
			]
		);
        TabManager->FindExistingLiveTab(CodeTabId)->GetParentDockTabStack()->SetCanDropToAttach(false);
        
        //Add native menu bar for the window on mac.
        TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuBarWidget);
        TabManager->UpdateMainMenu(nullptr, true);
PRAGMA_DISABLE_DEPRECATION_WARNINGS
        SaveLayoutTicker = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) {
            TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			TSingleton<ShProjectManager>::Get().SaveProject();
            return true;
        }), 2.0f);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Window->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			TSingleton<ShProjectManager>::Get().SaveProject();
			FSlateApplication::Get().RequestDestroyWindow(InWindow);
		}));
	}

    TSharedRef<SWidget> ShaderHelperEditor::SpawnStShaderPath(const FString& InStShaderPath)
    {
        auto PathContainer = SNew(SHorizontalBox);
        FString StShaderRelativePath = TSingleton<ShProjectManager>::Get().GetRelativePathToProject(InStShaderPath);
        TArray<FString> FileNames;
        StShaderRelativePath.ParseIntoArray(FileNames, TEXT("/"));
        FString RelativePathHierarchy;
        for(int32 i = 0; i < FileNames.Num(); i++)
        {
            FString FileName = FileNames[i];
            if(i == FileNames.Num() - 1)
            {
                PathContainer->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Center)
                .Padding(2.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FileName))
                ];
            }
            else
            {
                RelativePathHierarchy = FPaths::Combine(RelativePathHierarchy, FileName);
                PathContainer->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Center)
                .Padding(2.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(2.0f)
                    [
                        SNew(SImage)
                        .DesiredSizeOverride(FVector2D{12.0f, 12.0f})
                        .Image(FAppStyle::Get().GetBrush("Icons.FolderClosed"))
                    ]
                    +SHorizontalBox::Slot()
                    [
                        SNew(SButton)
                        .ContentPadding(FMargin{})
                        .ButtonStyle(&FShaderHelperStyle::Get().GetWidgetStyle< FButtonStyle >("SuperSimpleButton"))
                        .Text(FText::FromString(FileName + TEXT("  ❯")))
                        .OnClicked_Lambda([=]{
                            AssetBrowser->SetCurrentDisplyPath(TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(RelativePathHierarchy));
                            return FReply::Handled();
                        })
                    ]
                ];
            }

        }
        
        return SNew(SScrollBox)
            .Orientation(EOrientation::Orient_Horizontal)
            .ScrollBarVisibility(EVisibility::Collapsed)
            +SScrollBox::Slot()
            [
                PathContainer
            ];
    }

    void ShaderHelperEditor::Update(double DeltaTime)
    {
        
    }

    TSharedRef<SDockTab> ShaderHelperEditor::SpawnStShaderTab(const FSpawnTabArgs& Args)
    {
        FGuid StShaderGuid{Args.GetTabId().ToString()};
        FName TabId = Args.GetTabId().TabType;
        auto LoadedStShader = TSingleton<AssetManager>::Get().LoadAssetByGuid<StShader>(StShaderGuid);
        TSharedPtr<SShaderEditorBox> ShaderEditor;
        if(PendingStShadereTabs.Contains(LoadedStShader))
        {
            ShaderEditor = PendingStShadereTabs[LoadedStShader];
            PendingStShadereTabs.Remove(LoadedStShader);
        }
        else
        {
            ShaderEditor = SNew(SShaderEditorBox).StShaderAsset(LoadedStShader.Get());
        }
        auto NewStShaderTab = SNew(SShaderTab)
            .TabRole(ETabRole::DocumentTab)
            .Label(FText::FromString(LoadedStShader->GetFileName()))
            .OnTabClosed_Lambda([=](TSharedRef<SDockTab> ClosedTab) {
                CurProject->OpenedStShaders.Remove(*CurProject->OpenedStShaders.FindKey(ClosedTab));
                PendingStShadereTabs.Add(LoadedStShader, ShaderEditor);
                auto DockTabStack = ClosedTab->GetParentDockTabStack();
                PRAGMA_DISABLE_DEPRECATION_WARNINGS
                //Persist the tab being closed until next frame to be able to restore the tab.
                FDelegateHandle TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float) {
                    //If the StShader tab was not restored on the last frame.
                    if(PendingStShadereTabs.Contains(LoadedStShader))
                    {
                        CodeTabManager->UnregisterTabSpawner(TabId);
                        PendingStShadereTabs.Remove(LoadedStShader);
                        //Clear the PersistLayout when closing a StShader tab. we don't intend to restore it, so just destroy it.
                        DockTabStack->OnTabRemoved(Args.GetTabId());
                        TArray<TSharedRef<FTabManager::FArea>>& CollapsedDockAreas = GetPrivate_FTabManager_CollapsedDockAreas(*CodeTabManager);
                        CollapsedDockAreas.Empty();
                    }
                    FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
                    return false;
                }));
                PRAGMA_ENABLE_DEPRECATION_WARNINGS
            })
            [
                SNew(SVerticalBox)
                +SVerticalBox::Slot()
                .AutoHeight()
                [
                    SpawnStShaderPath(LoadedStShader->GetPath())
                ]
                +SVerticalBox::Slot()
                [
                    ShaderEditor.ToSharedRef()
                ]
            ];
        
        NewStShaderTab->SetTabIcon(LoadedStShader->GetImage());
        NewStShaderTab->SetOnTabRelocated(FSimpleDelegate::CreateLambda([this, NewStShaderTab = TWeakPtr<SShaderTab>{NewStShaderTab}] {
            if(NewStShaderTab.IsValid())
            {
                if (TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{ NewStShaderTab.Pin()->GetLayoutIdentifier() }, CodeTabMainArea.ToSharedRef()))
                {
                    StShaderTabStackInsertPoint = TabStack;
                }
            }
		}));
        NewStShaderTab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab> InTab, ETabActivationCause) {
            if(!CodeTabMainArea) return;
            
            if(TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{InTab->GetLayoutIdentifier()}, CodeTabMainArea.ToSharedRef()))
            {
				StShaderTabStackInsertPoint = TabStack;
            }
        }));
        CurProject->OpenedStShaders.Add(LoadedStShader, NewStShaderTab);
        return NewStShaderTab;
    }

	TSharedRef<SDockTab> ShaderHelperEditor::SpawnWindowTab(const FSpawnTabArgs& Args)
	{
		const FTabId& TabId = Args.GetTabId();
		TSharedPtr<SDockTab> SpawnedTab;

		if (TabId == PreviewTabId) {
            SpawnedTab = SNew(SDockTab);
			SpawnedTab->SetLabel(LOCALIZATION(PreviewTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Visible"));
			SpawnedTab->SetContent(
                SNew(SBorder)
                [
                    SNew(SViewport)
                    .ViewportInterface(ViewPort)
                ]
			
			);
		}
		else if (TabId == PropretyTabId) {
            SpawnedTab = SNew(SDockTab);
			SpawnedTab->SetLabel(LOCALIZATION(PropretyTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Info"));
			SpawnedTab->SetContent(
				SAssignNew(PropertyViewBox, SBox)
			);
		}
		else if (TabId == CodeTabId) {
            //Use the previous CodeTab when resetting whindow layout.
            if(CodeTab) {
                return CodeTab.ToSharedRef();
            }
            
            SpawnedTab = SAssignNew(CodeTab, SDockTab)
                .Visibility(EVisibility::Collapsed);
            
            CodeTabManager = FGlobalTabmanager::Get()->NewTabManager(SpawnedTab.ToSharedRef());
            
            for(const auto& [OpenedStShader, _] : CurProject->OpenedStShaders)
            {
                FName StShaderTabId{*OpenedStShader.GetGuid().ToString()};
                CodeTabManager->RegisterTabSpawner(StShaderTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnStShaderTab));
            }
 
            if(!CurProject->CodeTabLayout)
            {
                CurProject->CodeTabLayout = FTabManager::NewLayout("CodeTabLayout")
                ->AddArea
                (
                    FTabManager::NewPrimaryArea()
                    ->Split
                    (
                        FTabManager::NewStack()
                        ->AddTab(InitialInsertPointTabId, ETabState::ClosedTab)
                    )
                 ) , Args.GetOwnerWindow();
            }
            
            auto MenuBarBuilder = CreateMenuBarBuilder();
            auto MenuBarWidget = MenuBarBuilder.MakeWidget();
            CodeTabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuBarWidget);
            CodeTabManager->UpdateMainMenu(nullptr, true);
            
            SpawnedTab->SetContent(
               SNew(SOverlay)
               +SOverlay::Slot()
               [
                   SNew(SVerticalBox)
                   + SVerticalBox::Slot()
                   .VAlign(VAlign_Center)
                   .HAlign(HAlign_Center)
                   [
                       SNew(STextBlock)
                       .Text(LOCALIZATION("CodeTabTip"))
                       .Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
                   ]
               ]
               +SOverlay::Slot()
               [
                   CodeTabManager->RestoreFrom(CurProject->CodeTabLayout.ToSharedRef(), Args.GetOwnerWindow()).ToSharedRef()
               ]
           );
            CodeTabMainArea = CodeTabManager->FindPotentiallyClosedTab(InitialInsertPointTabId)->GetDockArea();
		}
		else if (TabId == AssetTabId) {
            SpawnedTab = SNew(SDockTab);
			SpawnedTab->SetLabel(LOCALIZATION(AssetTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Server"));
			SpawnedTab->SetContent(
				SAssignNew(AssetBrowser, SAssetBrowser)
				.ContentPathShowed(TSingleton<ShProjectManager>::Get().GetActiveContentDirectory())
                .State(&CurProject->AssetBrowserState)
			);
		}
		else if (TabId == GraphTabId) {
			SAssignNew(SpawnedTab, SDockTab)
			.Label(LOCALIZATION(GraphTabId.ToString()))
			[
				SAssignNew(GraphPanel, SGraphPanel)
				.GraphData(CurProject->Graph.Get())
			];
			SpawnedTab->SetTabIcon(FAppCommonStyle::Get().GetBrush("Icons.Graph"));
		}
		else {
			ensure(false);
		}
		return SpawnedTab.ToSharedRef();
	}

	void ShaderHelperEditor::ResetWindowLayout()
	{
        TabManager->UnregisterAllTabSpawners();
        for (const FName& TabId : TabIds)
        {
            TabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnWindowTab));
        }
        
        TabManager->CloseAllAreas();
        WindowContentBox->GetSlot(1).AttachWidget(TabManager->RestoreFrom(DefaultTabLayout.ToSharedRef(), Window).ToSharedRef());
        TabManager->FindExistingLiveTab(CodeTabId)->GetParentDockTabStack()->SetCanDropToAttach(false);
	}

    void ShaderHelperEditor::TryRestoreStShaderTab(AssetPtr<StShader> InStShader)
    {
        if(PendingStShadereTabs.Contains(InStShader))
        {
            FName StShaderTabId{*InStShader->GetGuid().ToString()};
            CodeTabManager->TryInvokeTab(StShaderTabId);
        }
    }

	void ShaderHelperEditor::OpenGraph(AssetPtr<Graph> InGraphData)
	{
		GraphPanel->SetGraphData(InGraphData.Get());
		CurProject->Graph = InGraphData;
	}

	void ShaderHelperEditor::OpenStShaderTab(AssetPtr<StShader> InStShader)
    {
        TSharedPtr<SDockTab>* TabPtr = CurProject->OpenedStShaders.Find(InStShader);
        if(TabPtr == nullptr || !*TabPtr)
        {
            FName StShaderTabId{*InStShader->GetGuid().ToString()};
            //Register the tab spawner to restore the tab if need.
            CodeTabManager->RegisterTabSpawner(StShaderTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnStShaderTab))
                .SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId&){ return nullptr; }));
            auto NewStShaderTab = SpawnStShaderTab({Window, StShaderTabId});
            if(StShaderTabStackInsertPoint.IsValid() && StShaderTabStackInsertPoint.Pin()->GetAllChildTabs().IsValidIndex(0))
            {
                auto FirstTab = StShaderTabStackInsertPoint.Pin()->GetAllChildTabs()[0];
                CodeTabManager->InsertNewDocumentTab(FirstTab->GetLayoutIdentifier().TabType, StShaderTabId, FTabManager::FLiveTabSearch{}, NewStShaderTab, true);
            }
            else
            {
                CodeTabManager->InsertNewDocumentTab(InitialInsertPointTabId, StShaderTabId, FTabManager::FRequireClosedTab{}, NewStShaderTab, true);
            }
          
        }
        else
        {
            (*TabPtr)->ActivateInParent(ETabActivationCause::SetDirectly);
            FSlateApplication::Get().SetAllUserFocus(*TabPtr);
        }
    }

	ShaderHelperEditor::WindowLayoutConfigInfo ShaderHelperEditor::LoadWindowLayout(const FString& InWindowLayoutConfigFileName)
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

	void ShaderHelperEditor::SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout)
	{
		TSharedRef<FJsonObject> RootJsonObject = MakeShared<FJsonObject>();

		TSharedRef<FJsonObject> RootWindowSizeJsonObject = MakeShared<FJsonObject>();
		Vector2D CurClientSize = Window->GetClientSizeInScreen();
		CurClientSize -= Vector2D{2, 10};
		RootWindowSizeJsonObject->SetNumberField(TEXT("Width"), CurClientSize.X);
		RootWindowSizeJsonObject->SetNumberField(TEXT("Height"), CurClientSize.Y);

		TSharedRef<FJsonObject> RootWindowPosJsonObject = MakeShared<FJsonObject>();
		Vector2D CurPos = Window->GetPositionInScreen();
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
			FFileHelper::SaveStringToFile(NewJsonContents, *WindowLayoutFileName);
		}
	}

	void ShaderHelperEditor::OnViewportResize(const Vector2f& InSize)
	{
		Renderer->OnViewportResize(InSize);
		ViewPort->SetViewPortRenderTexture(Renderer->GetFinalRT());
	}

    FMenuBarBuilder ShaderHelperEditor::CreateMenuBarBuilder()
	{
		FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());
		MenuBarBuilder.AddPullDownMenu(
			LOCALIZATION("File"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("File"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCALIZATION("Config"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("Config"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCALIZATION("Window"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("Window"))
		);
        return MenuBarBuilder;
	}

	void ShaderHelperEditor::FillMenu(FMenuBuilder& MenuBuilder, FString MenuName)
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
            if(!TabManager) {
                return false;
            }
            
			return TabManager->FindExistingLiveTab(TabId).IsValid();
		};

		if (MenuName == "File") {
			MenuBuilder.BeginSection("Project", FText::FromString("Project"));
			{
				MenuBuilder.AddMenuEntry(LOCALIZATION("New"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(

					));
				MenuBuilder.AddMenuEntry(LOCALIZATION("OpenEx"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
				
					));
				MenuBuilder.AddMenuEntry(LOCALIZATION("Save"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([&] { 
							TSingleton<ShProjectManager>::Get().SaveProject();
						})
					));
			}
			MenuBuilder.EndSection();
			
		}
		else if (MenuName == "Config") {
			MenuBuilder.AddSubMenu(LOCALIZATION("Language"), FText::GetEmpty(), FNewMenuDelegate::CreateLambda(
				[this](FMenuBuilder& MenuBuilder) {
					for (int Index{}; Index != static_cast<int>(SupportedLanguage::Num); Index++)
					{
						SupportedLanguage Lang{ Index };
						MenuBuilder.AddMenuEntry(
							LOCALIZATION(magic_enum::enum_name(Lang).data()),
							FText::GetEmpty(), FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda(
									[this, Lang]()
									{
										Editor::SetLanguage(Lang);
                                        TabManager->UpdateMainMenu(nullptr, true);
									}),
								FCanExecuteAction(),
								FIsActionChecked::CreateLambda(
									[Lang]()
									{
										return Editor::CurLanguage == Lang;
									})
							),
							NAME_None,
							EUserInterfaceActionType::ToggleButton
						);
					}
				}),
				false, FSlateIcon(FShaderHelperStyle::Get().GetStyleSetName(), "Icons.World"));
		}
		else if (MenuName == "Window") {
			MenuBuilder.BeginSection("WindowTabControl", FText::FromString("Tab Control"));
			{
				for (const FName& TabId : WindowMenuTabIds)
				{
					MenuBuilder.AddMenuEntry(LOCALIZATION(TabId.ToString()), FText::GetEmpty(),
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
				MenuBuilder.AddMenuEntry(LOCALIZATION("ResetWindowLayout"), FText::GetEmpty(),
					FSlateIcon{ FAppStyle::Get().GetStyleSetName(), "Icons.Refresh" }, 
					FUIAction(
						FExecuteAction::CreateRaw(this, &ShaderHelperEditor::ResetWindowLayout)
					));
			}
			MenuBuilder.EndSection();
		}
	}

}
