#include "CommonHeader.h"
#include "ShaderHelperEditor.h"
#include "App/ShaderHelperApp.h"
#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "magic_enum.hpp"
#include <Framework/Docking/SDockingTabStack.h>
#include "UI/Widgets/SShaderTab.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include <DesktopPlatformModule.h>
#include "Editor/AssetEditor/AssetEditor.h"
#include "UI/Widgets/Log/SOutputLog.h"
#include "UI/Widgets/Timeline/STimeline.h"

STEAL_PRIVATE_MEMBER(FTabManager, TArray<TSharedRef<FTabManager::FArea>>, CollapsedDockAreas)

using namespace FW;

namespace SH 
{
	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";

	const FName CodeTabId = "Code";
    const FName InitialInsertPointTabId = "CodeInsertPoint";

    const FName AssetTabId = "Asset";
	const FName GraphTabId = "Graph";
    const FName LogTabId = "Log";

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId, GraphTabId, LogTabId
	};

    const TArray<FName> WindowMenuTabIds{
        PreviewTabId, PropretyTabId, AssetTabId, GraphTabId, LogTabId
    };

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
	{
		CurProject = TSingleton<ShProjectManager>::Get().GetProject();
		ViewPort = MakeShared<PreviewViewPort>();
	}

	ShaderHelperEditor::~ShaderHelperEditor()
	{
		Renderer->ClearRenderComp();
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		FTicker::GetCoreTicker().RemoveTicker(SaveLayoutTicker);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
        if(FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().DestroyWindowImmediately(Window.ToSharedRef());
        }
		
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
                        ->AddTab(LogTabId, ETabState::OpenedTab)
                        ->SetForegroundTab(AssetTabId)
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
				FString ProjectPath = CurProject->GetFilePath();
                int Fps = FMath::RoundToInt(1 / GApp->GetDeltaTime());
				return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + FString::Printf(TEXT(" [%s] Fps:%d"), *ProjectPath, Fps));
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
			.FillHeight(0.965f)
			[
				TabManager->RestoreFrom(UsedLayout, Window).ToSharedRef()
			]
            +SVerticalBox::Slot()
            .FillHeight(0.035f)
            [
                SNew(STimeline).bStop(&CurProject->TimelineStop)
                    .CurTime(&CurProject->TimelineCurTime).MaxTime(&CurProject->TimelineMaxTime)
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
			CurProject->Save();
            return true;
        }), 2.0f);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Window->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			if (CurProject->AnyPendingAsset() && MessageDialog::Open(MessageDialog::OkCancel, Window, LOCALIZATION("SaveAssetTip")))
			{
				CurProject->SavePendingAssets();
				CurProject->Save();
			}
			else
			{
				CurProject->Save();
			}
			auto ShApp = static_cast<ShaderHelperApp*>(GApp.Get());
			ShApp->Launcher.Reset();
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
                        .ButtonStyle(&FAppCommonStyle::Get().GetWidgetStyle< FButtonStyle >("SuperSimpleButton"))
                        .Text(FText::FromString(FileName + TEXT("  ❯")))
                        .OnClicked_Lambda([RelativePathHierarchy, this]{
                            AssetBrowser->SetCurrentDisplyPath(TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(RelativePathHierarchy));
                            return FReply::Handled();
                        })
                    ]
                ];
            }

        }
        
        return PathContainer;
    }

    void ShaderHelperEditor::UpdateStShaderPath(const FString& InStShaderPath)
    {
        AssetObject* Asset = TSingleton<AssetManager>::Get().FindLoadedAsset(InStShaderPath);
        if(Asset)
        {
            StShader* Shader = static_cast<StShader*>(Asset);
            if(auto* PathBox = StShaderPathBoxMap.Find(Shader))
            {
                (*PathBox)->ClearChildren();
                (*PathBox)->AddSlot()
                [
                    SpawnStShaderPath(Shader->GetPath())
                ];
            }
        }
    }

    TSharedRef<SDockTab> ShaderHelperEditor::SpawnStShaderTab(const FSpawnTabArgs& Args)
    {
        FGuid StShaderGuid{Args.GetTabId().ToString()};
        FName TabId = Args.GetTabId().TabType;
        auto LoadedStShader = TSingleton<AssetManager>::Get().LoadAssetByGuid<StShader>(StShaderGuid);

        ShaderEditor = SNew(SShaderEditorBox).StShaderAsset(LoadedStShader);
        
        auto PathBox = SNew(SScrollBox)
            .Orientation(EOrientation::Orient_Horizontal)
            .ScrollBarVisibility(EVisibility::Collapsed)
            +SScrollBox::Slot()
            [
                SpawnStShaderPath(LoadedStShader->GetPath())
            ];
        StShaderPathBoxMap.Add(LoadedStShader, PathBox);
        
        auto NewStShaderTab = SNew(SShaderTab)
            .TabRole(ETabRole::DocumentTab)
			.Label_Lambda([this, LoadedStShader] {
				FString DirtyChar;
				if (CurProject->IsPendingAsset(LoadedStShader))
				{
					DirtyChar = "*";
				}
				return FText::FromString(LoadedStShader->GetFileName() + DirtyChar);
			})
            .OnTabClosed_Lambda([this, TabId, Args](TSharedRef<SDockTab> ClosedTab) {
                auto StShaderAsset = *CurProject->OpenedStShaders.FindKey(ClosedTab);
                CurProject->OpenedStShaders.Remove(StShaderAsset);
                StShaderPathBoxMap.Remove(StShaderAsset);
				//Clear the PersistLayout when closing a StShader tab. we don't intend to restore it, so just destroy it.
				auto DockTabStack = ClosedTab->GetParentDockTabStack();
				DockTabStack->OnTabRemoved(Args.GetTabId());
				TArray<TSharedRef<FTabManager::FArea>>& CollapsedDockAreas = GetPrivate_FTabManager_CollapsedDockAreas(*CodeTabManager);
				CollapsedDockAreas.Empty();
            })
            [
                SNew(SVerticalBox)
                +SVerticalBox::Slot()
                .AutoHeight()
                [
                    PathBox
                ]
                +SVerticalBox::Slot()
                [
                    SNew(SBorder)
                    [
                        ShaderEditor.ToSharedRef()
                    ]
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
        NewStShaderTab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this, LoadedStShader](TSharedRef<SDockTab> InTab, ETabActivationCause) {
            if(!CodeTabMainArea) return;
            GetShObjectOp(LoadedStShader)->OnSelect(LoadedStShader);
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

        if(TabId == LogTabId)
        {
            SpawnedTab = SNew(SDockTab)
            .Label(LOCALIZATION(LogTabId.ToString()))
            [
                SNew(SOutputLog)
            ];
            SpawnedTab->SetTabIcon(FAppCommonStyle::Get().GetBrush("Icons.Log"));
        }
		else if (TabId == PreviewTabId) {
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
                SNew(SBorder)
                .Padding(FMargin{2,2,3,2})
                [
                    SAssignNew(PropertyView, SPropertyView)
                    .ObjectData(CurPropertyObject)
                ]
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
                FName StShaderTabId{*OpenedStShader->GetGuid().ToString()};
				CodeTabManager->RegisterTabSpawner(
					StShaderTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnStShaderTab));
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
			.Label_Lambda([this] {
				FString DirtyChar;
				if (CurProject->IsPendingAsset(CurProject->Graph))
				{
					DirtyChar = "*";
				}
				return FText::FromString(LOCALIZATION(GraphTabId.ToString()).ToString() + DirtyChar);
			})
			[
                SNew(SBorder)
                [
                    SNew(SOverlay)
                    +SOverlay::Slot()
                    [
                        SAssignNew(GraphPanel, SGraphPanel)
                    ]
                    +SOverlay::Slot()
                    .VAlign(VAlign_Top)
                    .HAlign(HAlign_Left)
                    .Padding(6.0f)
                    [
                        SNew(STextBlock)
                        .Visibility(EVisibility::HitTestInvisible)
                        .ColorAndOpacity(FLinearColor::White)
                        .Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
                        .ShadowOffset(FVector2D{2,2})
                        .Text_Lambda([this] {
                            return CurProject->Graph ? FText::FromString("> " + CurProject->Graph->GetFileName() + "." + CurProject->Graph->FileExtension()) : FText{};
                        })
                    ]
                ]
		
			];
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Blueprints"));
			if (CurProject->Graph)
			{
				AssetOp::OpenAsset(CurProject->Graph);
			}
			
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

	void ShaderHelperEditor::OpenGraph(AssetPtr<Graph> InGraphData, TSharedPtr<RenderComponent> InGraphRenderComp)
	{
		Renderer->UnRegisterRenderComp(GraphRenderComp.Get());
		GraphRenderComp = InGraphRenderComp;
		if (GraphRenderComp)
		{
			Renderer->RegisterRenderComp(GraphRenderComp.Get());
		}

		GraphPanel->SetGraphData(InGraphData);
		CurProject->Graph = InGraphData;
        
        if(!InGraphData)
        {
            ViewPort->Clear();
        }
	}

    void ShaderHelperEditor::RefreshProperty()
    {
        PropertyView->Refresh();
    }

    void ShaderHelperEditor::ShowProperty(ShObject* InObjectData)
    {
        CurPropertyObject = InObjectData;
        PropertyView->SetObjectData(InObjectData);
    }

	void ShaderHelperEditor::OpenStShaderTab(AssetPtr<StShader> InStShader)
    {
        TSharedPtr<SDockTab>* TabPtr = CurProject->OpenedStShaders.Find(InStShader);
        if(TabPtr == nullptr || !*TabPtr)
        {
            FName StShaderTabId{*InStShader->GetGuid().ToString()};
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
            NewStShaderTab->ActivateInParent(ETabActivationCause::SetDirectly);
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
				MenuBuilder.AddMenuEntry(LOCALIZATION("Project"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
							FExecuteAction::CreateLambda([this] {
								auto ShApp = static_cast<ShaderHelperApp*>(GApp.Get());
								ShApp->Launcher = MakeShared<ProjectLauncher<ShProject>>([this, ShApp] {
									ShApp->AppEditor = MakeUnique<ShaderHelperEditor>(ShApp->GetClientSize(), static_cast<ShRenderer*>(ShApp->GetRenderer()));
									static_cast<ShaderHelperEditor*>(ShApp->AppEditor.Get())->InitEditorUI();
								});
								ShApp->Launcher->SetBeforeLaunchFunc(FSimpleDelegate::CreateLambda([this] {
									if (CurProject->AnyPendingAsset() && MessageDialog::Open(MessageDialog::OkCancel, Window, LOCALIZATION("SaveAssetTip")))
									{
										CurProject->SavePendingAssets();
										CurProject->Save();
									}
									else
									{
										CurProject->Save();
									}
								}));
							})
					));
				MenuBuilder.AddMenuEntry(LOCALIZATION("SaveAll"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this] { 
							CurProject->SavePendingAssets();
							CurProject->Save();
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
