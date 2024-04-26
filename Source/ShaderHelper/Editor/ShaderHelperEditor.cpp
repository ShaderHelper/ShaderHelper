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

using namespace FRAMEWORK;

namespace SH 
{
	const FString DefaultProjectPath = PathHelper::WorkspaceDir() / TEXT("TemplateProject/Default/Default.shprj");

	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";
	const FName CodeTabId = "Code";
	const FName AssetTabId = "Asset";

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId
	};

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
	{
		FShaderHelperStyle::Init();
		TSingleton<ShProjectManager>::Get().OpenProject(DefaultProjectPath);

		ViewPort = MakeShared<PreviewViewPort>();
		ViewPort->OnViewportResize.AddRaw(this, &ShaderHelperEditor::OnViewportResize);

		EditorStateSaveFileName = TSingleton<ShProjectManager>::Get().GetActiveSavedDirectory() / TEXT("EditorState.json");
		LoadEditorState(EditorStateSaveFileName);
		InitEditorUI();
	}

	ShaderHelperEditor::~ShaderHelperEditor()
	{
		FShaderHelperStyle::ShutDown();
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
					->SetSizeCoefficient(0.15f)
					->AddTab(PropretyTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.45f)
					->AddTab(CodeTabId, ETabState::OpenedTab)
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

		Window->SetContent(
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				CreateMenuBar()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				TabManager->RestoreFrom(UsedLayout, Window).ToSharedRef()
			]
		);

		Window->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			if (!bReInitEditor) { TabManager->SavePersistentLayout(); }
			FSlateApplication::Get().RequestDestroyWindow(InWindow);
		}));

		Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&) {
			OnWindowClosed.ExecuteIfBound(bReInitEditor);
		}));
	}

	TSharedRef<SDockTab> ShaderHelperEditor::SpawnWindowTab(const FSpawnTabArgs& Args)
	{
		const FTabId& TabId = Args.GetTabId();
		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);

		if (TabId == PreviewTabId) {
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
			SpawnedTab->SetLabel(LOCALIZATION(PropretyTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Info"));
			SpawnedTab->SetContent(
				SAssignNew(PropertyViewBox, SBox)
			);
		}
		else if (TabId == CodeTabId) {
			SpawnedTab->SetLabel(LOCALIZATION(CodeTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Edit"));
			SpawnedTab->SetContent(
				SNew(SShaderEditorBox)
				.Renderer(Renderer)
			);
		}
		else if (TabId == AssetTabId) {
			SpawnedTab->SetLabel(LOCALIZATION(AssetTabId.ToString()));
			SpawnedTab->SetTabIcon(FAppStyle::Get().GetBrush("Icons.Server"));
			SpawnedTab->SetContent(
				SNew(SAssetBrowser)
				.ContentPathShowed(TSingleton<ShProjectManager>::Get().GetActiveContentDirectory())
				.InitialSelectedDirectory(CurEditorState.SelectedDirectory)
				.InitialDirectoriesToExpand(CurEditorState.DirectoriesToExpand)
				.OnSelectedDirectoryChanged_Lambda([this](const FString& NewSelectedDirectory) {
					CurEditorState.SelectedDirectory = NewSelectedDirectory;
					SaveEditorState();
				})
				.OnExpandedDirectoriesChanged_Lambda([this](const TArray<FString>& NewExpandedDirectories) {
					CurEditorState.DirectoriesToExpand = NewExpandedDirectories;
					SaveEditorState();
				})
			);
		}
		else {
			ensure(false);
		}
		return SpawnedTab;
	}

	void ShaderHelperEditor::ResetWindowLayout()
	{
		//Clean window layout cache to use default layout.
		IFileManager::Get().Delete(*WindowLayoutFileName);
		bReInitEditor = true;
		Window->RequestDestroyWindow();
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

	TSharedRef<SWidget> ShaderHelperEditor::CreateMenuBar()
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
        
        TSharedRef<SWidget> MenuWidget = MenuBarBuilder.MakeWidget();
        //Add native menu bar for the window on mac.
        TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuWidget);
        TabManager->UpdateMainMenu(nullptr, true);
        
        return MenuWidget;
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
				[](FMenuBuilder& MenuBuilder) {
					for (int Index{}; Index != static_cast<int>(SupportedLanguage::Num); Index++)
					{
						SupportedLanguage Lang{ Index };
						MenuBuilder.AddMenuEntry(
							LOCALIZATION(magic_enum::enum_name(Lang).data()),
							FText::GetEmpty(), FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda(
									[Lang]()
									{
										Editor::SetLanguage(Lang);
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
				for (const FName& TabId : TabIds)
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
		SelectedDirectory =  TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(
			InJson->GetStringField("SelectedRelativeDirectory")
		);

		TArray<TSharedPtr<FJsonValue>> JsonRelativeDirectories = InJson->GetArrayField("RelativeDirectoriesToExpand");
		for (const auto& JsonRelativeDirectory : JsonRelativeDirectories)
		{
			DirectoriesToExpand.Add(
				TSingleton<ShProjectManager>::Get().ConvertRelativePathToFull(JsonRelativeDirectory->AsString())
			);
		}
	}

	TSharedRef<FJsonObject> ShaderHelperEditor::EditorState::ToJson() const
	{
		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
		JsonObject->SetStringField("SelectedRelativeDirectory", 
			TSingleton<ShProjectManager>::Get().GetRelativePathToProject(SelectedDirectory));

		TArray<TSharedPtr<FJsonValue>> JsonRelativeDirectories;
		for (const FString& Directory : DirectoriesToExpand)
		{
			TSharedPtr<FJsonValue> JsonRelativeDriectory = MakeShared<FJsonValueString>(
				TSingleton<ShProjectManager>::Get().GetRelativePathToProject(Directory));
			JsonRelativeDirectories.Add(MoveTemp(JsonRelativeDriectory));
		}

		JsonObject->SetArrayField("RelativeDirectoriesToExpand", MoveTemp(JsonRelativeDirectories));
		return JsonObject;
	}

}
