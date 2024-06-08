#include "CommonHeader.h"
#include "ShaderHelperEditor.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Property/PropertyView/SShaderPassPropertyView.h"
#include "ProjectManager/ShProjectManager.h"
#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "magic_enum.hpp"
#include <Framework/Docking/SDockingTabStack.h>
#include "UI/Widgets/SShaderPassTab.h"

STEAL_PRIVATE_MEMBER(FTabManager, TArray<TSharedRef<FTabManager::FArea>>, CollapsedDockAreas)

using namespace FRAMEWORK;

namespace SH 
{
	const FString DefaultProjectPath = PathHelper::WorkspaceDir() / TEXT("TemplateProject/Default/Default.shprj");

	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";

	const FName CodeTabId = "Code";
    const FName InitialInsertPointTabId = "CodeInsertPoint";

    const FName AssetTabId = "Asset";

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId,
	};

    const TArray<FName> WindowMenuTabIds{
        PreviewTabId, PropretyTabId, AssetTabId,
    };

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
	{
		TSingleton<ShProjectManager>::Get().OpenProject(DefaultProjectPath);

		ViewPort = MakeShared<PreviewViewPort>();
		ViewPort->OnViewportResize.AddRaw(this, &ShaderHelperEditor::OnViewportResize);

		EditorStateSaveFileName = TSingleton<ShProjectManager>::Get().GetActiveSavedDirectory() / TEXT("EditorState.json");
        
		LoadEditorState(EditorStateSaveFileName);
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
					->SetSizeCoefficient(0.45f)
					->AddTab(CodeTabId, ETabState::OpenedTab)
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
			.Title(LOCALIZATION("ShaderHelper"))
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
            CurEditorState.CodeTabLayout = CodeTabManager->PersistLayout();
            SaveEditorState();
            return true;
        }), 2.0f);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Window->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			TabManager->SavePersistentLayout();
            CurEditorState.CodeTabLayout = CodeTabManager->PersistLayout();
            SaveEditorState();
			FSlateApplication::Get().RequestDestroyWindow(InWindow);
		}));
	}

    TSharedRef<SWidget> ShaderHelperEditor::SpawnShaderPassPath(const FString& InShaderPassPath)
    {
        auto PathContainer = SNew(SHorizontalBox);
        FString ShaderPassRelativePath = TSingleton<ShProjectManager>::Get().GetRelativePathToProject(InShaderPassPath);
        TArray<FString> FileNames;
        ShaderPassRelativePath.ParseIntoArray(FileNames, TEXT("/"));
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

    TSharedRef<SDockTab> ShaderHelperEditor::SpawnShaderPassTab(const FSpawnTabArgs& Args)
    {
        FGuid ShaderPassGuid{Args.GetTabId().ToString()};
        FName TabId = Args.GetTabId().TabType;
        auto LoadedShaderPass = TSingleton<AssetManager>::Get().LoadAssetByGuid<ShaderPass>(ShaderPassGuid);
        TSharedPtr<SShaderEditorBox> ShaderEditor;
        if(PendingShaderPasseTabs.Contains(LoadedShaderPass))
        {
            ShaderEditor = PendingShaderPasseTabs[LoadedShaderPass];
            PendingShaderPasseTabs.Remove(LoadedShaderPass);
        }
        else
        {
            ShaderEditor = SNew(SShaderEditorBox).ShaderPassAsset(LoadedShaderPass.Get());
        }
        auto NewShaderPassTab = SNew(SShaderPassTab)
            .TabRole(ETabRole::DocumentTab)
            .Label(FText::FromString(LoadedShaderPass->GetFileName()))
            .OnTabClosed_Lambda([=](TSharedRef<SDockTab> ClosedTab) {
                CurEditorState.OpenedShaderPasses.Remove(*CurEditorState.OpenedShaderPasses.FindKey(ClosedTab));
                PendingShaderPasseTabs.Add(LoadedShaderPass, ShaderEditor);
                auto DockTabStack = ClosedTab->GetParentDockTabStack();
                PRAGMA_DISABLE_DEPRECATION_WARNINGS
                //Persist the tab being closed until next frame to be able to restore the tab.
                FDelegateHandle TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float) {
                    //If the shaderpass tab was not restored on the last frame.
                    if(PendingShaderPasseTabs.Contains(LoadedShaderPass))
                    {
                        CodeTabManager->UnregisterTabSpawner(TabId);
                        PendingShaderPasseTabs.Remove(LoadedShaderPass);
                        //Clear the PersistLayout when closing a shaderpass tab. we don't intend to restore it, so just destroy it.
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
                    SpawnShaderPassPath(LoadedShaderPass->GetPath())
                ]
                +SVerticalBox::Slot()
                [
                    ShaderEditor.ToSharedRef()
                ]
            ];
        
        NewShaderPassTab->SetTabIcon(LoadedShaderPass->GetImage());
        NewShaderPassTab->SetOnTabRelocated(FSimpleDelegate::CreateLambda([this, NewShaderPassTab = TWeakPtr<SShaderPassTab>{NewShaderPassTab}] {
            if(NewShaderPassTab.IsValid())
            {
                if (TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{ NewShaderPassTab.Pin()->GetLayoutIdentifier() }, CodeTabMainArea.ToSharedRef()))
                {
                    ShaderPassTabStackInsertPoint = TabStack;
                }
            }
		}));
        NewShaderPassTab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab> InTab, ETabActivationCause) {
            if(!CodeTabMainArea) return;
            
            if(TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{InTab->GetLayoutIdentifier()}, CodeTabMainArea.ToSharedRef()))
            {
				ShaderPassTabStackInsertPoint = TabStack;
            }
        }));
        CurEditorState.OpenedShaderPasses.Add(LoadedShaderPass, NewShaderPassTab);
        return NewShaderPassTab;
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
            
            for(const auto& [OpenedShaderPass, _] : CurEditorState.OpenedShaderPasses)
            {
                FName ShaderPassTabId{*OpenedShaderPass.GetGuid().ToString()};
                CodeTabManager->RegisterTabSpawner(ShaderPassTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnShaderPassTab));
            }
 
            if(!CurEditorState.CodeTabLayout)
            {
                CurEditorState.CodeTabLayout = FTabManager::NewLayout("CodeTabLayout")
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
                   CodeTabManager->RestoreFrom(CurEditorState.CodeTabLayout.ToSharedRef(), Args.GetOwnerWindow()).ToSharedRef()
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
                .State(CurEditorState.AssetBrowserState)
			);
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

    void ShaderHelperEditor::TryRestoreShaderPassTab(AssetPtr<ShaderPass> InShaderPass)
    {
        if(PendingShaderPasseTabs.Contains(InShaderPass))
        {
            FName ShaderPassTabId{*InShaderPass->GetGuid().ToString()};
            CodeTabManager->TryInvokeTab(ShaderPassTabId);
        }
    }

    void ShaderHelperEditor::OpenShaderPassTab(AssetPtr<ShaderPass> InShaderPass)
    {
        TSharedPtr<SDockTab>* TabPtr = CurEditorState.OpenedShaderPasses.Find(InShaderPass);
        if(TabPtr == nullptr || !*TabPtr)
        {
            FName ShaderPassTabId{*InShaderPass->GetGuid().ToString()};
            //Register the tab spawner to restore the tab if need.
            CodeTabManager->RegisterTabSpawner(ShaderPassTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnShaderPassTab))
                .SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId&){ return nullptr; }));
            auto NewShaderPassTab = SpawnShaderPassTab({Window, ShaderPassTabId});
            if(ShaderPassTabStackInsertPoint.IsValid() && ShaderPassTabStackInsertPoint.Pin()->GetAllChildTabs().IsValidIndex(0))
            {
                auto FirstTab = ShaderPassTabStackInsertPoint.Pin()->GetAllChildTabs()[0];
                CodeTabManager->InsertNewDocumentTab(FirstTab->GetLayoutIdentifier().TabType, ShaderPassTabId, FTabManager::FLiveTabSearch{}, NewShaderPassTab, true);
            }
            else
            {
                CodeTabManager->InsertNewDocumentTab(InitialInsertPointTabId, ShaderPassTabId, FTabManager::FRequireClosedTab{}, NewShaderPassTab, true);
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

	void ShaderHelperEditor::LoadEditorState(const FString& InFile)
	{
		FString JsonContent;
		if (FFileHelper::LoadFileToString(JsonContent, *InFile))
		{
			TSharedPtr<FJsonObject> EditorStateJsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
			FJsonSerializer::Deserialize(Reader, EditorStateJsonObject);
			CurEditorState.InitFromJson(EditorStateJsonObject);
		}

	}

	void ShaderHelperEditor::SaveEditorState()
	{
		TSharedRef<FJsonObject> EditorStateJsonObject = CurEditorState.ToJson();
		FString NewJsonContents;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NewJsonContents);
		if (FJsonSerializer::Serialize(EditorStateJsonObject, Writer))
		{
			FFileHelper::SaveStringToFile(NewJsonContents, *EditorStateSaveFileName);
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
				MenuBuilder.AddMenuEntry(LOCALIZATION("NewEx"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(

					));
				MenuBuilder.AddMenuEntry(LOCALIZATION("OpenEx"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(

					));
				MenuBuilder.AddMenuEntry(LOCALIZATION("Save"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([] { TSingleton<ShProjectManager>::Get().SaveProject(); })
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

	void ShaderHelperEditor::EditorState::InitFromJson(const TSharedPtr<FJsonObject>& InJson)
	{
        AssetBrowserState->AssetViewState.AssetViewSize = (float)InJson->GetNumberField("AssetViewSize");
        AssetBrowserState->DirectoryTreeState.CurSelectedDirectory =  TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(
			InJson->GetStringField("SelectedRelativeDirectory")
		);

		TArray<TSharedPtr<FJsonValue>> JsonRelativeDirectories = InJson->GetArrayField("RelativeDirectoriesToExpand");
		for (const auto& JsonRelativeDirectory : JsonRelativeDirectories)
		{
            AssetBrowserState->DirectoryTreeState.DirectoriesToExpand.Add(
				TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(JsonRelativeDirectory->AsString())
			);
		}
        TArray<TSharedPtr<FJsonValue>> JsonOpenedShaderPasses = InJson->GetArrayField("OpenedShaderPasses");
        for(const auto& JsonOpenedShaderPass : JsonOpenedShaderPasses)
        {
            auto LoadedShaderPass = TSingleton<AssetManager>::Get().LoadAssetByGuid<ShaderPass>(FGuid(JsonOpenedShaderPass->AsString()));
            if(LoadedShaderPass)
            {
                OpenedShaderPasses.Add(MoveTemp(LoadedShaderPass), nullptr);
            }
        }
        
        TSharedPtr<FJsonObject> CodeTabLayoutJsonObject = InJson->GetObjectField(TEXT("CodeTabLayout"));
        CodeTabLayout = FTabManager::FLayout::NewFromJson(MoveTemp(CodeTabLayoutJsonObject));
	}

	TSharedRef<FJsonObject> ShaderHelperEditor::EditorState::ToJson() const
	{
		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
		JsonObject->SetNumberField("AssetViewSize", AssetBrowserState->AssetViewState.AssetViewSize);
		JsonObject->SetStringField("SelectedRelativeDirectory", 
			TSingleton<ShProjectManager>::Get().GetRelativePathToProject(AssetBrowserState->DirectoryTreeState.CurSelectedDirectory));

		TArray<TSharedPtr<FJsonValue>> JsonRelativeDirectories;
		for (const FString& Directory : AssetBrowserState->DirectoryTreeState.DirectoriesToExpand)
		{
			TSharedPtr<FJsonValue> JsonRelativeDriectory = MakeShared<FJsonValueString>(
				TSingleton<ShProjectManager>::Get().GetRelativePathToProject(Directory));
			JsonRelativeDirectories.Add(MoveTemp(JsonRelativeDriectory));
		}
        JsonObject->SetArrayField("RelativeDirectoriesToExpand", JsonRelativeDirectories);
        
        TArray<TSharedPtr<FJsonValue>> JsonOpenedShaderPasses;
        for(const auto& [OpenedShaderPass, _] : OpenedShaderPasses)
        {
            TSharedPtr<FJsonValue> JsonShaderPassGuid = MakeShared<FJsonValueString>(OpenedShaderPass->GetGuid().ToString());
            JsonOpenedShaderPasses.Add(MoveTemp(JsonShaderPassGuid));
        }
        JsonObject->SetArrayField("OpenedShaderPasses", JsonOpenedShaderPasses);
        
        if(CodeTabLayout)
        {
            TSharedRef<FJsonObject> CodeTabLayoutJsonObject = CodeTabLayout->ToJson();
            JsonObject->SetObjectField("CodeTabLayout", CodeTabLayoutJsonObject);
        }
  
        
		return JsonObject;
	}

}
