#include "CommonHeader.h"
#include "ShaderHelperEditor.h"
#include "App/ShaderHelperApp.h"
#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include <Framework/Docking/SDockingTabStack.h>
#include "UI/Widgets/SShaderTab.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include <DesktopPlatformModule.h>
#include "Editor/AssetEditor/AssetEditor.h"
#include "UI/Widgets/Timeline/STimeline.h"
#include "UI/Widgets/Misc/CommonCommands.h"

STEAL_PRIVATE_MEMBER(FTabManager, TArray<TSharedRef<FTabManager::FArea>>, CollapsedDockAreas)

using namespace FW;

namespace SH 
{
	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId, GraphTabId, LogTabId,
		LocalTabId, GlobalTabId, CallStackTabId, WatchTabId
	};

    const TArray<FName> WindowMenuTabIds{
        PreviewTabId, PropretyTabId, AssetTabId, GraphTabId, LogTabId,
		LocalTabId, GlobalTabId, CallStackTabId, WatchTabId
    };

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
	{
		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			CommonCommands::Get().Continue,
			FExecuteAction::CreateLambda([this] {
				SShaderEditorBox* ShaderEditor = GetShaderEditor(CurDebuggableObject->GetShaderAsset());
				ShaderEditor->Continue();
				}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && DebuggerViewport->FinalizedPixel(); })
		);
		UICommandList->MapAction(
			CommonCommands::Get().StepInto,
			FExecuteAction::CreateLambda([this]{
			SShaderEditorBox* ShaderEditor = GetShaderEditor(CurDebuggableObject->GetShaderAsset());
			ShaderEditor->Continue(SShaderEditorBox::StepMode::StepInto);
			}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && DebuggerViewport->FinalizedPixel(); })
		);
		UICommandList->MapAction(
			CommonCommands::Get().StepOver,
			FExecuteAction::CreateLambda([this]{
				SShaderEditorBox* ShaderEditor = GetShaderEditor(CurDebuggableObject->GetShaderAsset());
				ShaderEditor->Continue(SShaderEditorBox::StepMode::StepOver);
			}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && DebuggerViewport->FinalizedPixel(); })
		);
		
		CurProject = TSingleton<ShProjectManager>::Get().GetProject();
		ViewPort = MakeShared<PreviewViewPort>();
		ViewPort->OniMouseChangeHandler = [this](const Vector4f&) { ForceRender(); };
	}

	ShaderHelperEditor::~ShaderHelperEditor()
	{
		Renderer->ClearRenderComp();
		FTicker::GetCoreTicker().RemoveTicker(SaveLayoutTicker);
        if(FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().DestroyWindowImmediately(MainWindow.ToSharedRef());
        }
		
	}

	void ShaderHelperEditor::CreateInternalWidgets()
	{
		SAssignNew(OutputLog, SOutputLog);
		
		SAssignNew(ViewportWidget, SViewport)
			.ViewportInterface(ViewPort)
			.Visibility_Lambda([this]{
				return IsDebugging ? EVisibility::Hidden : EVisibility::Visible;
			});
		SAssignNew(DebuggerViewport, SDebuggerViewport)
			.Visibility_Lambda([this]{
				return IsDebugging ? EVisibility::Visible : EVisibility::Hidden;
			});
		ViewPort->SetAssociatedWidget(ViewportWidget);
		
		SAssignNew(AssetBrowser, SAssetBrowser)
		.ContentPathShowed(TSingleton<ShProjectManager>::Get().GetActiveContentDirectory())
		.State(&CurProject->AssetBrowserState);
		
		SAssignNew(PropertyView, SPropertyView)
			.ObjectData(CurPropertyObject);
		
		SAssignNew(GraphPanel, SGraphPanel);
		if (CurProject->Graph)
		{
			AssetOp::OpenAsset(CurProject->Graph);
		}
		
		SAssignNew(DebuggerLocalVariableView, SDebuggerVariableView);
		SAssignNew(DebuggerGlobalVariableView, SDebuggerVariableView);
		SAssignNew(DebuggerWatchView, SDebuggerWatchView);
		SAssignNew(DebuggerCallStackView, SDebuggerCallStackView);
	}

	void ShaderHelperEditor::ForceRender()
	{
		if(TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop == true)
		{
			TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = false;
			Renderer->Render();
			TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
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
					->SetSizeCoefficient(0.32f)
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
					->SetSizeCoefficient(0.48f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(CodeTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->SetSizeCoefficient(0.3f)
						->Split
						(
							 FTabManager::NewStack()
							 ->SetSizeCoefficient(0.5f)
							 ->AddTab(CallStackTabId, ETabState::ClosedTab)
							 ->AddTab(AssetTabId, ETabState::OpenedTab)
							 ->AddTab(LogTabId, ETabState::OpenedTab)
							 ->SetForegroundTab(AssetTabId)
						)
						->Split
						(
							 FTabManager::NewStack()
							 ->SetSizeCoefficient(0.5f)
							 ->AddTab(LocalTabId, ETabState::ClosedTab)
							 ->AddTab(GlobalTabId, ETabState::ClosedTab)
							 ->AddTab(WatchTabId, ETabState::ClosedTab)
						)
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

		SAssignNew(MainWindow, SShWindow)
			.Title(TAttribute<FText>::CreateLambda([this] { 
				FString ProjectPath = CurProject->GetFilePath();
                int Fps = FMath::RoundToInt(1 / GApp->GetDeltaTime());
				return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + FString::Printf(TEXT(" [%s] Fps:%d"), *ProjectPath, Fps));
			}))
			.ScreenPosition(UsedWindowPos)
			.AutoCenter(AutoCenterRule)
			.ClientSize(UsedWindowSize);
		MainWindow->SetKeyDownHandler(FOnKeyDown::CreateLambda([this](const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
			//Process tool bar commands.
			if (UICommandList.IsValid())
			{
				UICommandList->ProcessCommandBindings(InKeyEvent);
			}
			return FReply::Handled();
		}));

		CreateInternalWidgets();
		TabManagerTab->AssignParentWidget(MainWindow);

		FSlateApplication::Get().AddWindow(MainWindow.ToSharedRef());
        
        auto MenuBarBuilder = CreateMenuBarBuilder();
        auto MenuBarWidget = MenuBarBuilder.MakeWidget();
		
		auto ToolBarBuilder = CreateToolBarBuilder();
		auto ToolBarWidget = ToolBarBuilder.MakeWidget();

		MainWindow->SetContent(
			SAssignNew(WindowContentBox, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
                MenuBarWidget
			]
		    +SVerticalBox::Slot()
		    .AutoHeight()
			[
				SNew(SBorder)
				.Padding(FMargin{0, 0.5f, 0, 1.0f})
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					.Padding(4)
					[
						ToolBarWidget
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(0.965f)
			[
				TabManager->RestoreFrom(UsedLayout, MainWindow).ToSharedRef()
			]
            +SVerticalBox::Slot()
            .FillHeight(0.035f)
            [
                SNew(STimeline).bStop(&CurProject->TimelineStop)
                    .CurTime(&CurProject->TimelineCurTime).MaxTime(&CurProject->TimelineMaxTime)
					.OnHandleMove([this](float){
						ForceRender();
					})
					.IsEnabled_Lambda([this] { return !IsDebugging; })
            ]
		);
        TabManager->FindExistingLiveTab(CodeTabId)->GetParentDockTabStack()->SetCanDropToAttach(false);
        
        //Add native menu bar for the window on mac.
        TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuBarWidget);
        TabManager->UpdateMainMenu(nullptr, true);

        SaveLayoutTicker = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) {
            TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			CurProject->Save();
            return true;
        }), 2.0f);

		MainWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			if(CurProject->AnyPendingAsset())
			{
				auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MainWindow, LOCALIZATION("SaveAssetsTip"));
				if(Ret == MessageDialog::MessageRet::Cancel)
				{
					return;
				}
				else if (Ret == MessageDialog::MessageRet::Ok)
				{
					CurProject->SavePendingAssets();
				}
			}
			
			TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			CurProject->Save();
			auto ShApp = static_cast<ShaderHelperApp*>(GApp.Get());
			ShApp->Launcher.Reset();
			FSlateApplication::Get().RequestDestroyWindow(InWindow);
		}));

		TSingleton<ShPluginManager>::Get().RegisterActivePlugins();
	}

    TSharedRef<SWidget> ShaderHelperEditor::SpawnShaderPath(const FString& InShaderPath)
    {
        auto PathContainer = SNew(SHorizontalBox);
        FString ShaderRelativePath = TSingleton<ShProjectManager>::Get().GetRelativePathToProject(InShaderPath);
        TArray<FString> FileNames;
        ShaderRelativePath.ParseIntoArray(FileNames, TEXT("/"));
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
							TabManager->TryInvokeTab(AssetTabId);
                            AssetBrowser->SetCurrentDisplyPath(TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(RelativePathHierarchy));
                            return FReply::Handled();
                        })
                    ]
                ];
            }

        }
        
        return PathContainer;
    }

    void ShaderHelperEditor::UpdateShaderPath(const FString& InShaderPath)
    {
        AssetObject* Asset = TSingleton<AssetManager>::Get().FindLoadedAsset(InShaderPath);
        if(Asset)
        {
            ShaderAsset* ShaderAssetObj = static_cast<ShaderAsset*>(Asset);
            if(auto* PathBox = ShaderPathBoxMap.Find(ShaderAssetObj))
            {
                (*PathBox)->ClearChildren();
                (*PathBox)->AddSlot()
                [
                    SpawnShaderPath(ShaderAssetObj->GetPath())
                ];
            }
        }
    }

	void ShaderHelperEditor::InvokeDebuggerTabs()
	{
		TabManager->TryInvokeTab(GlobalTabId);
		TabManager->TryInvokeTab(WatchTabId);
		TabManager->TryInvokeTab(LocalTabId);
		TabManager->TryInvokeTab(CallStackTabId);
	}

	void ShaderHelperEditor::CloseDebuggerTabs()
	{
		if (auto LocalTab = TabManager->FindExistingLiveTab(LocalTabId))
		{
			LocalTab->RequestCloseTab();
		}
        if(auto GlobalTab = TabManager->FindExistingLiveTab(GlobalTabId))
        {
            GlobalTab->RequestCloseTab();
        }
		if(auto WatchTab = TabManager->FindExistingLiveTab(WatchTabId))
		{
			WatchTab->RequestCloseTab();
		}
		if(auto CallStackTab = TabManager->FindExistingLiveTab(CallStackTabId))
		{
			CallStackTab->RequestCloseTab();
		}
	}

    TSharedRef<SDockTab> ShaderHelperEditor::SpawnShaderTab(const FSpawnTabArgs& Args)
    {
        FGuid ShaderGuid{Args.GetTabId().ToString()};
		FTabId TabId = Args.GetTabId();
        auto LoadedShader = TSingleton<AssetManager>::Get().LoadAssetByGuid<ShaderAsset>(ShaderGuid);

        auto ShaderEditor = SNew(SShaderEditorBox).ShaderAssetObj(LoadedShader);
        ShaderEditors.Add(LoadedShader, ShaderEditor);
        
        auto PathBox = SNew(SScrollBox)
            .Orientation(EOrientation::Orient_Horizontal)
            .ScrollBarVisibility(EVisibility::Collapsed)
            +SScrollBox::Slot()
            [
                SpawnShaderPath(LoadedShader->GetPath())
            ];
        ShaderPathBoxMap.Add(LoadedShader, PathBox);
        
        TSharedRef<SShaderTab> NewShaderTab = SNew(SShaderTab)
            .TabRole(ETabRole::DocumentTab)
			.Label_Lambda([this, LoadedShader] {
				FString DirtyChar;
				if (CurProject->IsPendingAsset(LoadedShader))
				{
					DirtyChar = "*";
				}
				return FText::FromString(LoadedShader->GetFileName() + DirtyChar);
			})
			.OnCanCloseTab_Lambda([this, TabId]{
				if(CurDebuggableObject && IsDebugging)
				{
					ShaderAsset* Shader = CurDebuggableObject->GetShaderAsset();
					if(CurProject->OpenedShaders[Shader]->GetLayoutIdentifier() == TabId)
					{
						auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MainWindow, LOCALIZATION("TerminateDebuggerTip"));
						if(Ret == MessageDialog::MessageRet::Ok)
						{
							EndDebugging();
						}
						else
						{
							return false;
						}
					}
				}
				return true;
			})
            .OnTabClosed_Lambda([this, TabId, Args](TSharedRef<SDockTab> ClosedTab) {
                auto ShaderAssetObj = *CurProject->OpenedShaders.FindKey(ClosedTab);
                CurProject->OpenedShaders.Remove(ShaderAssetObj);
                ShaderPathBoxMap.Remove(ShaderAssetObj);
                ShaderEditors.Remove(ShaderAssetObj);
				//Clear the PersistLayout when closing a Shader tab. we don't intend to restore it, so just destroy it.
				auto DockTabStack = ClosedTab->GetParentDockTabStack();
				DockTabStack->OnTabRemoved(TabId);
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
                        ShaderEditor
                    ]
                ]
            ];
        
        NewShaderTab->SetTabIcon(LoadedShader->GetImage());
        NewShaderTab->SetOnTabRelocated(FSimpleDelegate::CreateLambda([this, NewShaderTab = TWeakPtr<SShaderTab>{NewShaderTab}] {
            if(NewShaderTab.IsValid())
            {
                if (TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{ NewShaderTab.Pin()->GetLayoutIdentifier() }, CodeTabMainArea.ToSharedRef()))
                {
                    ShaderTabStackInsertPoint = TabStack;
                }
            }
		}));
        NewShaderTab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this, LoadedShader](TSharedRef<SDockTab> InTab, ETabActivationCause InCause) {
            if(!CodeTabMainArea) return;
            GetShObjectOp(LoadedShader)->OnSelect(LoadedShader);
            if(TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{InTab->GetLayoutIdentifier()}, CodeTabMainArea.ToSharedRef()))
            {
				ShaderTabStackInsertPoint = TabStack;
            }
            
            if(InCause == ETabActivationCause::UserClickedOnTab)
            {
                ShaderEditors[LoadedShader]->SetFocus();
            }
            
        }));
        CurProject->OpenedShaders.Add(LoadedShader, NewShaderTab);
        return NewShaderTab;
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
				OutputLog.ToSharedRef()
            ];
            SpawnedTab->SetTabIcon(FAppCommonStyle::Get().GetBrush("Icons.Log"));
        }
		else if(TabId == LocalTabId)
		{
			SpawnedTab = SNew(SDockTab)
			.Label(LOCALIZATION(LocalTabId.ToString()))
			[
				DebuggerLocalVariableView.ToSharedRef()
			];
			SpawnedTab->SetTabIcon(FShaderHelperStyle::Get().GetBrush("Icons.Variable"));
		}
		else if(TabId == GlobalTabId)
		{
			SpawnedTab = SNew(SDockTab)
			.Label(LOCALIZATION(GlobalTabId.ToString()))
			[
				DebuggerGlobalVariableView.ToSharedRef()
			];
			SpawnedTab->SetTabIcon(FShaderHelperStyle::Get().GetBrush("Icons.Variable"));
		}
		else if(TabId == WatchTabId)
		{
			SpawnedTab = SNew(SDockTab)
			.Label(LOCALIZATION(WatchTabId.ToString()))
			[
				DebuggerWatchView.ToSharedRef()
			];
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Visible"));
		}
		else if(TabId == CallStackTabId)
		{
			SpawnedTab = SNew(SDockTab)
			.Label(LOCALIZATION(CallStackTabId.ToString()))
			[
				DebuggerCallStackView.ToSharedRef()
			];
			SpawnedTab->SetTabIcon(FShaderHelperStyle::Get().GetBrush("Icons.CallStack"));
		}
		else if (TabId == PreviewTabId) {
            SpawnedTab = SNew(SDockTab);
			SpawnedTab->SetLabel(LOCALIZATION(PreviewTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Visible"));
			SpawnedTab->SetContent(
                SNew(SBorder)
                [
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						ViewportWidget.ToSharedRef()
					]
					+SOverlay::Slot()
					[
						DebuggerViewport.ToSharedRef()
					]
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
					PropertyView.ToSharedRef()
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
            
            for(const auto& [OpenedShader, _] : CurProject->OpenedShaders)
            {
                FName ShaderTabId{*OpenedShader->GetGuid().ToString()};
				CodeTabManager->RegisterTabSpawner(
					ShaderTabId, FOnSpawnTab::CreateRaw(this, &ShaderHelperEditor::SpawnShaderTab));
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
			SpawnedTab->SetContent(AssetBrowser.ToSharedRef());
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
                        GraphPanel.ToSharedRef()
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
        WindowContentBox->GetSlot(2).AttachWidget(TabManager->RestoreFrom(DefaultTabLayout.ToSharedRef(), MainWindow).ToSharedRef());
        TabManager->FindExistingLiveTab(CodeTabId)->GetParentDockTabStack()->SetCanDropToAttach(false);
	}

	void ShaderHelperEditor::OpenGraph(AssetPtr<Graph> InGraphData, TSharedPtr<RenderComponent> InGraphRenderComp)
	{
		if(CurProject->Graph && CurProject->Graph->IsDirty())
		{
			auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MainWindow, LOCALIZATION("SaveAssetTip"));
			if(Ret == MessageDialog::MessageRet::Cancel)
			{
				return;
			}
			else if (Ret == MessageDialog::MessageRet::Ok)
			{
				CurProject->Graph->Save();
			}
			else
			{
				CurProject->Graph->MarkDirty(false);
			}
		}
		
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

	void ShaderHelperEditor::OpenShaderTab(AssetPtr<ShaderAsset> InShader)
    {
        TSharedPtr<SDockTab>* TabPtr = CurProject->OpenedShaders.Find(InShader);
        if(TabPtr == nullptr || !*TabPtr)
        {
            FName ShaderTabId{*InShader->GetGuid().ToString()};
            auto NewShaderTab = SpawnShaderTab({ MainWindow, ShaderTabId});
            if(ShaderTabStackInsertPoint.IsValid() && ShaderTabStackInsertPoint.Pin()->GetAllChildTabs().IsValidIndex(0))
            {
                auto FirstTab = ShaderTabStackInsertPoint.Pin()->GetAllChildTabs()[0];
                CodeTabManager->InsertNewDocumentTab(FirstTab->GetLayoutIdentifier().TabType, ShaderTabId, FTabManager::FLiveTabSearch{}, NewShaderTab, true);
            }
            else
            {
                CodeTabManager->InsertNewDocumentTab(InitialInsertPointTabId, ShaderTabId, FTabManager::FRequireClosedTab{}, NewShaderTab, true);
            }
            NewShaderTab->ActivateInParent(ETabActivationCause::SetDirectly);
        }
        else
        {
            (*TabPtr)->ActivateInParent(ETabActivationCause::SetDirectly);
            ShaderEditors[InShader]->SetFocus();
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
		Vector2D CurClientSize = MainWindow->GetClientSizeInScreen() / MainWindow->GetDPIScaleFactor();
		RootWindowSizeJsonObject->SetNumberField(TEXT("Width"), CurClientSize.X);
		RootWindowSizeJsonObject->SetNumberField(TEXT("Height"), CurClientSize.Y);

		TSharedRef<FJsonObject> RootWindowPosJsonObject = MakeShared<FJsonObject>();
		Vector2D CurPos = MainWindow->GetPositionInScreen() / MainWindow->GetDPIScaleFactor();
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

	void ShaderHelperEditor::EndDebugging()
	{
		IsDebugging = false;
		CurDebuggableObject->OnEndDebuggging();
	}

	void ShaderHelperEditor::StartDebugging()
	{		
		TRefCountPtr<GpuTexture> DebugTarget = CurDebuggableObject->OnStartDebugging();
		if(DebugTarget)
		{
			IsDebugging = true;
			DebuggerViewport->SetDebugTarget(MoveTemp(DebugTarget));
			auto User0 = FSlateApplication::Get().GetUser(0);
			User0->LockCursor(DebuggerViewport.ToSharedRef());
		}
	}

	FToolBarBuilder ShaderHelperEditor::CreateToolBarBuilder()
	{
		FToolBarBuilder ToolBarBuilder(TSharedPtr<FUICommandList>(), FMultiBoxCustomization::None, nullptr);
		ToolBarBuilder.SetStyle(&FShaderHelperStyle::Get(), FName("Toolbar.ShaderHelper"));
		ToolBarBuilder.AddToolBarButton(
			FExecuteAction{},
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FAppStyle::Get().GetStyleSetName(), "Icons.ArrowLeft"),
			EUserInterfaceActionType::Button
		);
		ToolBarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([] { return false; })
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FAppStyle::Get().GetStyleSetName(), "Icons.ArrowRight"),
			EUserInterfaceActionType::Button
		);
		FToolBarBuilder DebuggerToolBarBuilder(UICommandList, FMultiBoxCustomization::None, nullptr);
		DebuggerToolBarBuilder.SetStyle(&FShaderHelperStyle::Get(), FName("Toolbar.ShaderHelper"));
		
		DebuggerToolBarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction::CreateLambda([this]{
					if(IsDebugging)
					{
						EndDebugging();
					}
					else
					{
						StartDebugging();
					}
				}),
				FCanExecuteAction::CreateLambda([this] {
					return CurDebuggableObject != nullptr;
				})
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			TAttribute<FSlateIcon>::CreateLambda([this] {
				if(IsDebugging) {
					return FSlateIcon(FShaderHelperStyle::Get().GetStyleSetName(), "Icons.Pause");
				}
				return FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.Bug");
			}),
			EUserInterfaceActionType::Button
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			FUIAction(
				  FExecuteAction::CreateLambda([this]{
					  SShaderEditorBox* ShaderEditor = GetShaderEditor(CurDebuggableObject->GetShaderAsset());
					  ShaderEditor->Continue();
				  }),
				FCanExecuteAction::CreateLambda([this] { return IsDebugging && DebuggerViewport->FinalizedPixel(); })
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.ArrowBoldRight"),
			EUserInterfaceActionType::Button
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			CommonCommands::Get().StepOver,
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.StepOver")
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			CommonCommands::Get().StepInto,
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.StepInto")
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([] { return false; })
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.StepBack"),
			EUserInterfaceActionType::Button
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([] { return false; })
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.StepOut"),
			EUserInterfaceActionType::Button
		);
		ToolBarBuilder.AddToolBarWidget(
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Black"))
			.Padding(1)
			[
				DebuggerToolBarBuilder.MakeWidget()
			]
		);
		ToolBarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([] { return false; })
			),
			NAME_None,
			FText::GetEmpty(), FText::GetEmpty(),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.FullText"),
			EUserInterfaceActionType::Button
		);
		return ToolBarBuilder;
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
			LOCALIZATION("Edit"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("Edit"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCALIZATION("Window"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("Window"))
		);
		MenuBarBuilder.AddPullDownMenu(
			LOCALIZATION("Help"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateRaw(this, &ShaderHelperEditor::FillMenu, FString("Help"))
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
								ShApp->Launcher->SetCanLaunchFunc([this] {
									if(CurProject->AnyPendingAsset())
									{
										auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MainWindow, LOCALIZATION("SaveAssetsTip"));
										if(Ret == MessageDialog::MessageRet::Cancel)
										{
											return false;
										}
										else if (Ret == MessageDialog::MessageRet::Ok)
										{
											CurProject->SavePendingAssets();
										}
									}
									CurProject->Save();
									return true;
								});
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
		else if (MenuName == "Edit") {
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
										return Editor::GetLanguage() == Lang;
									})
							),
							NAME_None,
							EUserInterfaceActionType::ToggleButton
						);
					}
				}),
				false, FSlateIcon(FShaderHelperStyle::Get().GetStyleSetName(), "Icons.World"));
			MenuBuilder.AddMenuEntry(LOCALIZATION("Preferences"), FText::GetEmpty(), FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Settings"),
				FUIAction(
					FExecuteAction::CreateLambda([this] {
						if (!PreferenceWindow.IsValid())
						{
							auto NewWindow = SNew(SShWindow).Title_Lambda([this] {
								return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + "-" + LOCALIZATION("Preferences").ToString());
							})
							.ClientSize({600, 400})
							[
								SAssignNew(PreferenceView, SPreferenceView)
							];
							NewWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
								PreferenceView->SavePersistentState();
							}));
							PreferenceWindow = NewWindow;
							FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, MainWindow.ToSharedRef());
						}
					}
				)));
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
		else if (MenuName == "Help")
		{
			MenuBuilder.AddMenuEntry(LOCALIZATION("About"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([this] {

						}
					)));
		}
	}

}
