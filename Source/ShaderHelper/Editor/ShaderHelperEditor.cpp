#include "CommonHeader.h"
#include "ShaderHelperEditor.h"
#include "App/ShaderHelperApp.h"
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderAsset.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/Texture3D.h"
#include "AssetObject/Model.h"
#include "AssetObject/Render/Render.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "Common/Path/PathHelper.h"
#include "Debugger/DebuggableObject.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "Editor/EditorCommands.h"
#include "Editor/PreviewViewPort.h"
#include "CodeEditorCommands.h"
#include "DebuggerViewCommands.h"
#include "SceneViewCommands.h"
#include "PluginManager/ShPluginManager.h"
#include "ProjectManager/ShProjectManager.h"
#include "Renderer/RenderComponent.h"
#include "Renderer/ShRenderer.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "RenderResource/Mesh.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Debugger/SFragmentDebuggerViewport.h"
#include "UI/Widgets/Debugger/SComputeDebuggerViewport.h"
#include "UI/Widgets/Debugger/SVertexDebuggerViewport.h"
#include "UI/Widgets/Debugger/SDebuggerVariableView.h"
#include "UI/Widgets/Debugger/SDebuggerCallStackView.h"
#include "UI/Widgets/Debugger/SDebuggerWatchView.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include "UI/Widgets/Log/SOutputLog.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Widgets/Misc/SShWindow.h"
#include "UI/Widgets/Preference/SPreferenceView.h"
#include "UI/Widgets/Scene/SSceneView.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/SShaderTab.h"
#include "UI/Widgets/Timeline/STimeline.h"
#include "magic_enum.hpp"

#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>
#include <Framework/Docking/SDockingTabStack.h>
#include <DesktopPlatformModule.h>
#include <Framework/Notifications/NotificationManager.h>
#include <Widgets/Notifications/SNotificationList.h>
#include <Widgets/Input/SHyperlink.h>
#include <Widgets/Input/SSegmentedControl.h>
#include <Widgets/SViewport.h>

STEAL_PRIVATE_MEMBER(FTabManager, TArray<TSharedRef<FTabManager::FArea>>, CollapsedDockAreas)

using namespace FW;

namespace SH 
{
	static const FString WindowLayoutFileName = PathHelper::SavedDir() / TEXT("WindowLayout.json");
	static constexpr const TCHAR* ChineseDocumentUrl = TEXT("https://github.com/SjMxr233/ShaderHelper/blob/main/docs/UserGuide.md");
	static constexpr const TCHAR* EnglishDocumentUrl = TEXT("https://github.com/SjMxr233/ShaderHelper/blob/main/docs/UserGuide.en.md");

	const TArray<FName> TabIds{
		PreviewTabId, PropretyTabId, CodeTabId, AssetTabId, GraphTabId, LogTabId,
		LocalTabId, GlobalTabId, CallStackTabId, WatchTabId, SceneTabId
	};

    const TArray<FName> WindowMenuTabIds{
        PreviewTabId, PropretyTabId, AssetTabId, GraphTabId, LogTabId,
		LocalTabId, GlobalTabId, CallStackTabId, WatchTabId, SceneTabId
    };

	TSharedPtr<SWindow> ShaderHelperEditor::GetMainWindow() const
	{
		return StaticCastSharedPtr<SWindow>(MainWindow);
	}

	TWeakPtr<SWindow> ShaderHelperEditor::GetPreferenceWindow() const
	{
		return StaticCastSharedPtr<SWindow>(PreferenceWindow.Pin());
	}

	GizmoMode ShaderHelperEditor::GetGizmoMode() const
	{
		return CurProject->GizmoMode;
	}

	void ShaderHelperEditor::SetGizmoMode(GizmoMode Mode)
	{
		CurProject->GizmoMode = Mode;
	}

	GizmoSpace ShaderHelperEditor::GetGizmoSpace() const
	{
		return CurProject->GizmoSpace;
	}

	bool ShaderHelperEditor::IsScenePreview() const
	{
		return CurProject->bScenePreview;
	}

	bool ShaderHelperEditor::IsPropertyLocked() const
	{
		return PropertyView->IsLocked();
	}

	DebuggableObject* ShaderHelperEditor::GetDebuggaleObject() const
	{
		return CurDebuggableObject.IsValid() ? dynamic_cast<DebuggableObject*>(CurDebuggableObject.Get()) : nullptr;
	}

	ShaderHelperEditor::ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer)
		: Renderer(InRenderer)
		, WindowSize(InWindowSize)
		, CurrentDebugItem(DebugItem::Pixel)
	{
		CodeEditorCommands::Register();
		DebuggerViewCommands::Register();
		SceneViewCommands::Register();
		EditorCommands::Register();

		UICommandList = MakeShared<FUICommandList>();
		UICommandList->MapAction(
			CodeEditorCommands::Get().Debug,
			FExecuteAction::CreateLambda([this] {
				if (IsDebugging)
				{
					EndDebugging();
				}
				else
				{
					StartDebugging();
				}
			}),
			FCanExecuteAction::CreateLambda([this] {
				DebuggableObject* Debuggable = GetDebuggaleObject();
				return Debuggable && Debuggable->GetShaderAsset(CurrentDebugItem) != nullptr;
			})
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().Continue,
			FExecuteAction::CreateLambda([this] {
				Continue();
			}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && IsFinalizedForCurrentItem(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().StepInto,
			FExecuteAction::CreateLambda([this]{
				Continue(StepMode::StepInto);
			}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && IsFinalizedForCurrentItem(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().StepOver,
			FExecuteAction::CreateLambda([this]{
				Continue(StepMode::StepOver);
			}),
			FCanExecuteAction::CreateLambda([this] { return IsDebugging && IsFinalizedForCurrentItem(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().GoBack,
			FExecuteAction::CreateLambda([this] {
				NavigationIndex--;
				const auto& [ShaderAssetObj, Location] = NavigationHistory[NavigationIndex];
				if (!ShaderAssetObj)
				{
					return;
				}
				OpenShaderTab(ShaderAssetObj);
				GetShaderEditor(ShaderAssetObj)->JumpTo(Location);
			}),
			FCanExecuteAction::CreateLambda([this] { return NavigationIndex > 0; }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			CodeEditorCommands::Get().GoForward,
			FExecuteAction::CreateLambda([this] {
				NavigationIndex++;
				const auto& [ShaderAssetObj, Location] = NavigationHistory[NavigationIndex];
				if (!ShaderAssetObj)
				{
					return;
				}
				OpenShaderTab(ShaderAssetObj);
				GetShaderEditor(ShaderAssetObj)->JumpTo(Location);
			}),
			FCanExecuteAction::CreateLambda([this] { return NavigationIndex < NavigationHistory.Num() - 1; }),
			EUIActionRepeatMode::RepeatEnabled
		);

		UICommandList->MapAction(
			EditorCommands::Get().Undo,
			FExecuteAction::CreateLambda([this] { DoUndoActive(); }),
			FCanExecuteAction::CreateLambda([this] { return CanUndoActive(); }),
			EUIActionRepeatMode::RepeatEnabled
		);
		UICommandList->MapAction(
			EditorCommands::Get().Redo,
			FExecuteAction::CreateLambda([this] { DoRedoActive(); }),
			FCanExecuteAction::CreateLambda([this] { return CanRedoActive(); }),
			EUIActionRepeatMode::RepeatEnabled
		);

		CurProject = TSingleton<ShProjectManager>::Get().GetProject();
		ViewPort = MakeShared<PreviewViewPort>();
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
		
		SAssignNew(ViewportWidget, SShViewport)
			.ViewportInterface(ViewPort)
			.Visibility_Lambda([this]{
				return IsDebugging ? EVisibility::Hidden : EVisibility::Visible;
			});
		SAssignNew(FragmentDebuggerViewport, SFragmentDebuggerViewport)
			.Visibility_Lambda([this]{
				return (IsDebugging && CurrentDebugItem == DebugItem::Pixel && !bShowingLinePreview) ? EVisibility::Visible : EVisibility::Hidden;
			});
		SAssignNew(VertexDebuggerViewport, SVertexDebuggerViewport)
			.Visibility_Lambda([this]{
				return (IsDebugging && CurrentDebugItem == DebugItem::Vertex && !bShowingLinePreview) ? EVisibility::Visible : EVisibility::Hidden;
			});
		SAssignNew(ComputeDebuggerViewport, SComputeDebuggerViewport)
			.Visibility_Lambda([this]{
				return (IsDebugging && CurrentDebugItem == DebugItem::Compute && !bShowingLinePreview) ? EVisibility::Visible : EVisibility::Hidden;
			});
		LinePreviewViewPort = MakeShared<FW::PreviewViewPort>();
		SAssignNew(LinePreviewWidget, SViewport)
			.ViewportInterface(LinePreviewViewPort)
			.Visibility_Lambda([this]{
				return (IsDebugging && bShowingLinePreview) ? EVisibility::Visible : EVisibility::Hidden;
			});
		ViewPort->SetAssociatedWidget(ViewportWidget);

		CreateBuiltinAssets();
		
		SAssignNew(AssetBrowser, SAssetBrowser)
		.ContentPathShowed(TSingleton<ShProjectManager>::Get().GetActiveContentDirectory())
		.BuiltInDir(PathHelper::BuiltinDir())
		.State(&CurProject->AssetBrowserState);
		
		SAssignNew(PropertyView, SPropertyView)
			.ObjectData(CurPropertyObject);
		
		SAssignNew(GraphPanel, SGraphPanel);
		SAssignNew(SceneView, SSceneView);
		if (CurProject->Graph)
		{
			AssetOp::OpenAsset(CurProject->Graph);
		}
		
		SAssignNew(DebuggerLocalVariableView, SDebuggerVariableView);
		SAssignNew(DebuggerGlobalVariableView, SDebuggerVariableView);
		SAssignNew(DebuggerWatchView, SDebuggerWatchView);
		SAssignNew(DebuggerCallStackView, SDebuggerCallStackView);
		SAssignNew(ShaderEditorTipWindow, SWindow)
			.Type(EWindowType::ToolTip)
			.CreateTitleBar(false)
			.IsTopmostWindow(true)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.IsPopupWindow(true)
			.SizingRule(ESizingRule::Autosized)
			.ActivationPolicy(EWindowActivationPolicy::Never);
		FSlateApplication::Get().AddWindow(ShaderEditorTipWindow.ToSharedRef(), false);
	}

	void ShaderHelperEditor::CreateBuiltinAssets()
	{
		FString BuiltInShaderToyDir = PathHelper::BuiltinDir() / TEXT("ShaderToy");
		if (!IFileManager::Get().DirectoryExists(*BuiltInShaderToyDir))
		{
			IFileManager::Get().MakeDirectory(*BuiltInShaderToyDir, true);

			FString ShaderToyResourceDir = PathHelper::ResourceDir() / TEXT("ShaderToy");

			// Import Texture2D assets from Texture subfolder
			FString TextureResourceDir = ShaderToyResourceDir / TEXT("Texture");
			if (IFileManager::Get().DirectoryExists(*TextureResourceDir))
			{
				FString TextureSaveDir = BuiltInShaderToyDir / TEXT("Texture");
				IFileManager::Get().MakeDirectory(*TextureSaveDir, true);

				TArray<FString> TextureFiles;
				IFileManager::Get().FindFiles(TextureFiles, *(TextureResourceDir / TEXT("*")), true, false);
				for (const FString& FileName : TextureFiles)
				{
					FString FilePath = TextureResourceDir / FileName;
					AssetImporter* Importer = GetAssetImporter(FilePath);
					if (Importer)
					{
						TUniquePtr<AssetObject> Asset = Importer->CreateAssetObject(FilePath);
						if (Asset)
						{
							FString SavedFilePath = TextureSaveDir / (FPaths::GetBaseFilename(FileName) + TEXT(".") + Asset->FileExtension());
							TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
							Asset->ObjectName = FText::FromString(FPaths::GetBaseFilename(FileName));
							if (Texture2D* Texture2dAsset = dynamic_cast<Texture2D*>(Asset.Get()))
							{
								Texture2dAsset->GenerateMipmap = true;
							}
							Asset->Serialize(*Ar);
						}
					}
				}
			}

			// Import TextureCube assets from Cubemap subfolder
			FString CubemapResourceDir = ShaderToyResourceDir / TEXT("Cubemap");
			if (IFileManager::Get().DirectoryExists(*CubemapResourceDir))
			{
				FString CubemapSaveDir = BuiltInShaderToyDir / TEXT("Cubemap");
				IFileManager::Get().MakeDirectory(*CubemapSaveDir, true);

				const TCHAR* FaceSuffixes[] = { TEXT("_Right"), TEXT("_Left"), TEXT("_Top"), TEXT("_Bottom"), TEXT("_Front"), TEXT("_Back") };

				TArray<FString> CubemapDirs;
				IFileManager::Get().FindFiles(CubemapDirs, *(CubemapResourceDir / TEXT("*")), false, true);

				for (const FString& CubemapDirName : CubemapDirs)
				{
					FString CubemapDir = CubemapResourceDir / CubemapDirName;
					TArray<FString> FaceFiles;
					IFileManager::Get().FindFiles(FaceFiles, *(CubemapDir / TEXT("*")), true, false);

					TArray<TArray<uint8>> FaceData;
					FaceData.SetNum(6);
					uint32 CubeSize = 0;
					GpuFormat CubeFormat = GpuFormat::B8G8R8A8_UNORM;
					bool bAllFacesValid = true;

					for (int32 FaceIndex = 0; FaceIndex < 6 && bAllFacesValid; FaceIndex++)
					{
						FString* FoundFile = FaceFiles.FindByPredicate([&](const FString& FileName) {
							return FPaths::GetBaseFilename(FileName).EndsWith(FaceSuffixes[FaceIndex]);
						});

						if (!FoundFile)
						{
							bAllFacesValid = false;
							break;
						}

						FString FaceFilePath = CubemapDir / *FoundFile;
						AssetImporter* Importer = GetAssetImporter(FaceFilePath);
						if (!Importer)
						{
							bAllFacesValid = false;
							break;
						}

						TUniquePtr<AssetObject> FaceAsset = Importer->CreateAssetObject(FaceFilePath);
						Texture2D* FaceTex = static_cast<Texture2D*>(FaceAsset.Get());
						if (!FaceTex || FaceTex->GetWidth() != FaceTex->GetHeight())
						{
							bAllFacesValid = false;
							break;
						}

						if (FaceIndex == 0)
						{
							CubeSize = FaceTex->GetWidth();
							CubeFormat = FaceTex->GetFormat();
						}
						else if (FaceTex->GetWidth() != CubeSize || FaceTex->GetFormat() != CubeFormat)
						{
							bAllFacesValid = false;
							break;
						}

						FaceData[FaceIndex] = FaceTex->GetRawData();
					}

					if (bAllFacesValid && CubeSize > 0)
					{
						auto CubeAsset = MakeUnique<TextureCube>(CubeSize, CubeFormat, FaceData);
						FString SavedFilePath = CubemapSaveDir / (CubemapDirName + TEXT(".") + CubeAsset->FileExtension());
						TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
						CubeAsset->ObjectName = FText::FromString(CubemapDirName);
						CubeAsset->GenerateMipmap = true;
						CubeAsset->Serialize(*Ar);
					}
				}
			}

			// Import Texture3D assets from Volume subfolder (.bin format)
			FString VolumeResourceDir = ShaderToyResourceDir / TEXT("Volume");
			if (IFileManager::Get().DirectoryExists(*VolumeResourceDir))
			{
				FString VolumeSaveDir = BuiltInShaderToyDir / TEXT("Volume");
				IFileManager::Get().MakeDirectory(*VolumeSaveDir, true);

				TArray<FString> VolumeFiles;
				IFileManager::Get().FindFiles(VolumeFiles, *(VolumeResourceDir / TEXT("*.bin")), true, false);
				for (const FString& FileName : VolumeFiles)
				{
					FString FilePath = VolumeResourceDir / FileName;
					TArray<uint8> FileData;
					if (!FFileHelper::LoadFileToArray(FileData, *FilePath)) continue;

					// .bin header: magic "BIN\0" (4 bytes) + uint32 width + uint32 height + uint32 depth + uint32 channels
					if (FileData.Num() < 20) continue;
					const uint8* Header = FileData.GetData();
					if (Header[0] != 'B' || Header[1] != 'I' || Header[2] != 'N' || Header[3] != '\0') continue;

					uint32 VolWidth = *reinterpret_cast<const uint32*>(Header + 4);
					uint32 VolHeight = *reinterpret_cast<const uint32*>(Header + 8);
					uint32 VolDepth = *reinterpret_cast<const uint32*>(Header + 12);
					uint32 Channels = *reinterpret_cast<const uint32*>(Header + 16);

					GpuFormat VolFormat;
					if (Channels == 1) VolFormat = GpuFormat::R8_UNORM;
					else if (Channels == 4) VolFormat = GpuFormat::B8G8R8A8_UNORM;
					else continue;

					const uint32 ExpectedDataSize = VolWidth * VolHeight * VolDepth * Channels;
					if ((uint32)FileData.Num() - 20 < ExpectedDataSize) continue;

					TArray<uint8> RawData;
					RawData.SetNum(VolWidth * VolHeight * VolDepth * 4);

					const uint8* SrcData = FileData.GetData() + 20;
					if (Channels == 4)
					{
						for (uint32 j = 0; j < VolWidth * VolHeight * VolDepth; j++)
						{
							RawData[j * 4 + 0] = SrcData[j * 4 + 2];
							RawData[j * 4 + 1] = SrcData[j * 4 + 1];
							RawData[j * 4 + 2] = SrcData[j * 4 + 0];
							RawData[j * 4 + 3] = SrcData[j * 4 + 3];
						}
					}
					else // Channels == 1, expand to RGBA
					{
						for (uint32 j = 0; j < VolWidth * VolHeight * VolDepth; j++)
						{
							RawData[j * 4 + 0] = SrcData[j];
							RawData[j * 4 + 1] = SrcData[j];
							RawData[j * 4 + 2] = SrcData[j];
							RawData[j * 4 + 3] = 255;
						}
						VolFormat = GpuFormat::B8G8R8A8_UNORM;
					}

					auto VolumeAsset = MakeUnique<Texture3D>(VolWidth, VolHeight, VolDepth, VolFormat, RawData);
					FString SavedFilePath = VolumeSaveDir / (FPaths::GetBaseFilename(FileName) + TEXT(".") + VolumeAsset->FileExtension());
					TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
					VolumeAsset->ObjectName = FText::FromString(FPaths::GetBaseFilename(FileName));
					VolumeAsset->GenerateMipmap = true;
					VolumeAsset->Serialize(*Ar);
				}
			}
		}
		// Create built-in Mesh Model assets (Cube, Sphere, Quad)
		FString BuiltInMeshDir = PathHelper::BuiltinDir() / TEXT("Mesh");
		if (!IFileManager::Get().DirectoryExists(*BuiltInMeshDir))
		{
			IFileManager::Get().MakeDirectory(*BuiltInMeshDir, true);

			struct BuiltinMeshInfo { const TCHAR* Name; TFunction<MeshData()> CreateFunc; };
			BuiltinMeshInfo BuiltinMeshes[] = {
				{ TEXT("Cube"),   [] { return CreateCube(); } },
				{ TEXT("Sphere"), [] { return CreateSphere(); } },
				{ TEXT("Quad"),   [] { return CreateQuad(); } },
			};

			for (const auto& MeshInfo : BuiltinMeshes)
			{
				auto MeshModel = MakeUnique<Model>(TArray<MeshData>{ MeshInfo.CreateFunc() });
				MeshModel->ObjectName = FText::FromString(MeshInfo.Name);
				FString SavedFilePath = BuiltInMeshDir / (FString(MeshInfo.Name) + TEXT(".") + MeshModel->FileExtension());
				TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
				MeshModel->Serialize(*Ar);
			}
		}

		TSingleton<AssetManager>::Get().MountProject(PathHelper::BuiltinDir());
	}

	void ShaderHelperEditor::ForceRender()
	{
		double CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - LastForceRenderTime < ForceRenderThrottleInterval)
		{
			return;
		}
		LastForceRenderTime = CurrentTime;

		ScopedTimelineResume TimelineResumeScope;
		Renderer->Render();
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
					->SetSizeCoefficient(0.53f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(CodeTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->SetSizeCoefficient(0.25f)
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
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.15f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(SceneTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(PropretyTabId, ETabState::OpenedTab)
					)
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
				return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + "-" + ANSI_TO_TCHAR(magic_enum::enum_name(GetGpuRhiBackendType()).data())
					+ FString::Printf(TEXT(" [%s] Fps:%d"), *ProjectPath, Fps));
			}))
			.ScreenPosition(UsedWindowPos)
			.AutoCenter(AutoCenterRule)
			.MinWidth(400)
			.MinHeight(400)
			.ClientSize(UsedWindowSize);

		CreateInternalWidgets();
		TabManagerTab->AssignParentWidget(MainWindow);

		FSlateApplication::Get().AddWindow(MainWindow.ToSharedRef());
		FSlateApplication::Get().SetUnhandledKeyDownEventHandler(FOnKeyEvent::CreateLambda([this](const FKeyEvent& InKeyEvent) {
			//Process tool bar commands.
			UICommandList->ProcessCommandBindings(InKeyEvent);
			return FReply::Handled();
		}));
        
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
						if (auto RenderComp = dynamic_cast<ShaderToyRenderComp*>(GraphRenderComp.Get()))
						{
							RenderComp->Context.bResetPreviousFrame = true;
							ForceRender();
							RenderComp->Context.bResetPreviousFrame = false;
						}
						else
						{
							ForceRender();
						}
					})
					.IsEnabled_Lambda([this] { return !IsDebugging; })
            ]
		);
        TabManager->FindExistingLiveTab(CodeTabId)->GetParentDockTabStack()->SetCanDropToAttach(false);
	
        SaveLayoutTicker = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) {
            TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			CurProject->Save();
            return true;
        }), 2.0f);

		MainWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([this](const TSharedRef<SWindow>& InWindow) {
			if(CurProject->AnyPendingAsset())
			{
				auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MessageDialog::Shocked, MainWindow, LOCALIZATION("SaveAssetsTip"));
				if(Ret == MessageDialog::MessageRet::Cancel)
				{
					return;
				}
				else if (Ret == MessageDialog::MessageRet::Ok)
				{
					CurProject->SavePendingAssets();
				}
			}
			if (IsDebugging)
			{
				EndDebugging();
			}
		
			TabManager->SavePersistentLayout();
            CurProject->CodeTabLayout = CodeTabManager->PersistLayout();
			CurProject->Save();
			auto ShApp = static_cast<ShaderHelperApp*>(GApp.Get());
			ShApp->Launcher.Reset();
			FSlateApplication::Get().RequestDestroyWindow(InWindow);
		}));

		TSingleton<ShPluginManager>::Get().UnregisterActivePlugins();
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
                .Padding(4.0f, 0.0f, 4.0f, 0.0f)
                [
                    SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .AutoWidth()
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					.VAlign(VAlign_Center)
                    [
                        SNew(SImage)
						.ColorAndOpacity(FStyleColors::Foreground)
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

	void ShaderHelperEditor::AddNavigationInfo(FW::AssetPtr<ShaderAsset> InShader, const FTextLocation& InLocation)
	{
		if (!InShader)
		{
			return;
		}

		if (NavigationHistory.Num() > 0)
		{			
			if (NavigationHistory[NavigationIndex].Key == InShader && 
				(NavigationHistory[NavigationIndex].Value == InLocation || NavigationHistory[NavigationIndex].Value == FTextLocation{} || FMath::Abs(NavigationHistory[NavigationIndex].Value.GetLineIndex() - InLocation.GetLineIndex()) < 8))
			{
				NavigationHistory[NavigationIndex] = {InShader, InLocation};
				return;
			}
		}

		NavigationIndex++;
		NavigationHistory.SetNum(NavigationIndex);
		NavigationHistory.Emplace(InShader, InLocation);

		if (NavigationHistory.Num() > MaxNavigation)
		{
			NavigationHistory.RemoveAt(0);
			NavigationIndex--;
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
			.IconColor(FLinearColor::White)
            .TabRole(ETabRole::DocumentTab)
			.Label_Lambda([this, LoadedShader] {
				FString DirtyChar;
				if (CurProject->IsPendingAsset(LoadedShader))
				{
					DirtyChar = "*";
				}
				return FText::FromString(DirtyChar + LoadedShader->GetFileName());
			})
			.OnCanCloseTab_Lambda([this, TabId]{
				if(DebuggableObject* Debuggable = GetDebuggaleObject(); Debuggable && IsDebugging)
				{
					ShaderAsset* Shader = Debuggable->GetShaderAsset(CurrentDebugItem);
					if(CurProject->OpenedShaders[Shader]->GetLayoutIdentifier() == TabId)
					{
						auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MessageDialog::Shocked, MainWindow, LOCALIZATION("TerminateDebuggerTip"));
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
				if (ShObjectOp* Op = GetShObjectOp(ShaderAssetObj))
				{
					Op->OnCancelSelect(ShaderAssetObj);
				}
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
			if (ShObjectOp* Op = GetShObjectOp(LoadedShader))
			{
				Op->OnSelect(LoadedShader);
			}
            if(TSharedPtr<SDockingTabStack> TabStack = CodeTabManager->FindTabInLiveArea(FTabMatcher{InTab->GetLayoutIdentifier()}, CodeTabMainArea.ToSharedRef()))
            {
				ShaderTabStackInsertPoint = TabStack;
            }
            
            if(InCause == ETabActivationCause::UserClickedOnTab)
            {
                ShaderEditors[LoadedShader]->SetFocus();
				auto ShaderEditor = GetShaderEditor(static_cast<ShaderAsset*>(LoadedShader));
				AddNavigationInfo(LoadedShader, ShaderEditor->ShaderMultiLineEditableText->GetCursorLocation());
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
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							ViewportWidget.ToSharedRef()
						]
						+SOverlay::Slot()
						[
							FragmentDebuggerViewport.ToSharedRef()
						]
						+SOverlay::Slot()
						[
							VertexDebuggerViewport.ToSharedRef()
						]
						+SOverlay::Slot()
						[
							ComputeDebuggerViewport.ToSharedRef()
						]
						+SOverlay::Slot()
						[
							LinePreviewWidget.ToSharedRef()
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.Padding(FMargin(6, 1))
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this] {
									FIntPoint Size = ViewPort->GetSize();
									return FText::FromString(FString::Printf(TEXT("%s: %d x %d"), *LOCALIZATION("ViewportSize").ToString(), Size.X, Size.Y));
								})
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(FMargin(12, 0, 0, 0))
							[
								SNew(STextBlock)
								.Text_Lambda([this] {
									Vector2f Pos = ViewPort->GetMousePos();
									return FText::FromString(FString::Printf(TEXT("%s: %d, %d"), *LOCALIZATION("MousePos").ToString(), (int32)Pos.x, (int32)Pos.y));
								})
							]
						]
					]
                ]

			);
		}
		else if (TabId == SceneTabId)
		{
			SpawnedTab = SNew(SDockTab);
			SpawnedTab->SetLabel(LOCALIZATION(SceneTabId.ToString()));
			SpawnedTab->SetTabIcon(FShaderHelperStyle::Get().GetBrush("Icons.World"));
			SpawnedTab->SetContent(
				SNew(SBorder)
				.Padding(FMargin{2,2,3,2})
				[
					SceneView.ToSharedRef()
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
                        .Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
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

	bool ShaderHelperEditor::OpenGraph(AssetPtr<Graph> InGraphData)
	{
		if(CurProject->Graph && CurProject->Graph->IsDirty() &&
			InGraphData != nullptr && InGraphData != CurProject->Graph)
		{
			auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MessageDialog::Shocked, MainWindow, LOCALIZATION("SaveAssetTip"));
			if(Ret == MessageDialog::MessageRet::Cancel)
			{
				return false;
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

		GraphPanel->SetGraphData(InGraphData);
		CurProject->Graph = InGraphData;
        ViewPort->Clear();
        
        if(!InGraphData)
        {
			SceneView->SetRender(nullptr);
        }
		else if (auto* RenderGraph = dynamic_cast<Render*>(InGraphData.Get()))
		{
			SceneView->SetRender(RenderGraph);
		}
		else
		{
			SceneView->SetRender(nullptr);
		}

		return true;
	}

	void ShaderHelperEditor::SetGraphRenderComp(TSharedPtr<RenderComponent> InGraphRenderComp)
	{
		Renderer->UnRegisterRenderComp(GraphRenderComp.Get());
		GraphRenderComp = MoveTemp(InGraphRenderComp);
		if (GraphRenderComp)
		{
			Renderer->RegisterRenderComp(GraphRenderComp.Get());
		}
		TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
	}

    void ShaderHelperEditor::RefreshProperty(bool bClear)
    {
		if (bClear)
		{
			CurPropertyObject.Reset();
		}
        PropertyView->Refresh(bClear);
    }

    void ShaderHelperEditor::ShowProperty(ShObject* InObjectData)
    {
		if (!PropertyView->IsLocked())
		{
			CurPropertyObject = InObjectData;
			PropertyView->SetObjectData(InObjectData);
		}
    }

	void ShaderHelperEditor::OpenShaderTab(AssetPtr<ShaderAsset> InShader)
    {
		if (!InShader)
		{
			return;
		}

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
			AddNavigationInfo(InShader, {});
        }
        else if(!(*TabPtr)->IsActive())
        {
            (*TabPtr)->ActivateInParent(ETabActivationCause::SetDirectly);
			AddNavigationInfo(InShader, GetShaderEditor(InShader)->ShaderMultiLineEditableText->GetCursorLocation());
        }

		GetShaderEditor(InShader)->SetFocus();
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
		DismissLinePreview();
		IsDebugging = false;
		GetDebuggaleObject()->OnEndDebuggging();
		Debugger.Reset();
		VertexDebuggerViewport->Clear();
		CloseDebuggerTabs();
	}

	void ShaderHelperEditor::SetDebuggableObject(FW::ShObject* InObject)
	{
		if (InObject)
		{
			check(dynamic_cast<DebuggableObject*>(InObject));
			CurDebuggableObject.SetReference(InObject);
		}
		else
		{
			CurDebuggableObject.Reset();
		}
		NormalizeCurrentDebugItem();
	}

	void ShaderHelperEditor::SetCurrentDebugItem(DebugItem InItem)
	{
		if (IsDebugging)
		{
			return;
		}
		CurrentDebugItem = InItem;
		NormalizeCurrentDebugItem();
	}

	void ShaderHelperEditor::NormalizeCurrentDebugItem()
	{
		if (IsDebugging)
		{
			return;
		}
		DebuggableObject* Debuggable = GetDebuggaleObject();
		if (!Debuggable)
		{
			CurrentDebugItem = DebugItem::Pixel;
			return;
		}

		const TArray<DebugItem> SupportedItems = Debuggable->GetSupportedDebugItems();
		if (SupportedItems.Contains(CurrentDebugItem))
		{
			return;
		}
		CurrentDebugItem = SupportedItems.Num() > 0 ? SupportedItems[0] : DebugItem::Pixel;
	}

	void ShaderHelperEditor::ShowLinePreview(const DebuggerLocation& Loc)
	{
		const auto& PreviewTextures = Debugger.GetLinePreviewTextures();
		if (const auto* TexPtr = PreviewTextures.Find(Loc))
		{
			LinePreviewViewPort->SetViewPortRenderTexture(TexPtr->GetReference());
			LinePreviewLocation = Loc;
			bShowingLinePreview = true;
		}
	}

	void ShaderHelperEditor::DismissLinePreview()
	{
		if (bShowingLinePreview)
		{
			bShowingLinePreview = false;
			LinePreviewLocation = {};
			LinePreviewViewPort->Clear();
		}
	}

	void ShaderHelperEditor::Continue(StepMode Mode)
	{
		if(Debugger.HasEditDuringDebugging())
		{
			auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MessageDialog::Shocked, GetMainWindow(), LOCALIZATION("EditDuringDebugging"));
			if(Ret == MessageDialog::MessageRet::Ok)
			{
				Debugger.ClearEditDuringDebugging();
			}
			else
			{
				return;
			}
		}
		
		if(Debugger.Continue(Mode))
		{
			Debugger.ShowDebuggerResult();
			FString StopFile = Debugger.GetStopLocation().File;
			ShaderAsset* MainShader = GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem);
			AssetPtr<ShaderAsset> StopShader = MainShader->FindIncludeAsset(StopFile);
			if (StopShader)
			{
				AssetOp::OpenAsset(StopShader);
				SShaderEditorBox* ShaderEditor = GetShaderEditor(StopShader);
				ShaderEditor->JumpTo(ShaderEditor->GetLineIndex(Debugger.GetStopLocation().LineNumber));
				ShaderEditor->UnFold(Debugger.GetStopLocation().LineNumber);
			}
			else
			{
				SShaderEditorBox* ShaderEditor = GetShaderEditor(MainShader);
				ShaderEditor->JumpTo(ShaderEditor->GetLineIndex(Debugger.GetStopLocation().LineNumber));
				ShaderEditor->UnFold(Debugger.GetStopLocation().LineNumber);
			}
		}
		else
		{
			EndDebugging();
		}
	}

	std::optional<Vector2u> ShaderHelperEditor::ValidateVertex(const InvocationState& InState)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());
		return Debugger.ValidateVertex(InState);
	}

	std::optional<Vector2u> ShaderHelperEditor::ValidatePixel(const InvocationState& InState)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());
		return Debugger.ValidatePixel(InState);
	}

	std::optional<TPair<Vector3u, Vector3u>> ShaderHelperEditor::ValidateCompute(const InvocationState& InState)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());
		return Debugger.ValidateCompute(InState);
	}

	void ShaderHelperEditor::DebugPixel(const Vector2u& InPixelCoord, const InvocationState& InState)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());
		
		GApp->EnqueueBusyTask([=, this](TFunction<void()> Done) {
			FNotificationInfo Info(LOCALIZATION("StartDebuggerTip"));
			Info.Image = FAppStyle::Get().GetBrush("NoBrush");
			Info.bFireAndForget = false;
			Info.FadeInDuration = 0.0f;
			Info.FadeOutDuration = 0.0f;
			auto Notification = FSlateNotificationManager::Get().AddNotification(Info);
			Notification->SetCompletionState(SNotificationItem::CS_Pending);
			Async(EAsyncExecution::Thread, [=, this]() {
				try
				{
					Debugger.DebugPixel(InPixelCoord, InState);
				}
				catch (const std::runtime_error& e)
				{
					AsyncTask(ENamedThreads::GameThread, [=, this] {
						Notification->Fadeout();
						Done();
						FText FailureInfo = LOCALIZATION("DebugFailure");
						SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo.ToString(), UTF8_TO_TCHAR(e.what()));
						MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GetMainWindow(), FailureInfo);
						EndDebugging();
					});
					return;
				}

				AsyncTask(ENamedThreads::GameThread, [=, this] {
					Notification->Fadeout();
					Done();
					Continue();
				});
			});
		});
	}

	void ShaderHelperEditor::DebugCompute(const Vector3u& InWorkGroupId, const Vector3u& InLocalInvocationId, const InvocationState& InState)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());

		GApp->EnqueueBusyTask([=, this](TFunction<void()> Done) {
			FNotificationInfo Info(LOCALIZATION("StartDebuggerTip"));
			Info.Image = FAppStyle::Get().GetBrush("NoBrush");
			Info.bFireAndForget = false;
			Info.FadeInDuration = 0.0f;
			Info.FadeOutDuration = 0.0f;
			auto Notification = FSlateNotificationManager::Get().AddNotification(Info);
			Notification->SetCompletionState(SNotificationItem::CS_Pending);
			Async(EAsyncExecution::Thread, [=, this]() {
				try
				{
					Debugger.DebugCompute(InWorkGroupId, InLocalInvocationId, InState);
				}
				catch (const std::runtime_error& e)
				{
					AsyncTask(ENamedThreads::GameThread, [=, this] {
						Notification->Fadeout();
						Done();
						FText FailureInfo = LOCALIZATION("DebugFailure");
						SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo.ToString(), UTF8_TO_TCHAR(e.what()));
						MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GetMainWindow(), FailureInfo);
						EndDebugging();
					});
					return;
				}

				AsyncTask(ENamedThreads::GameThread, [=, this] {
					Notification->Fadeout();
					Done();
					Continue();
				});
			});
		});
	}

	void ShaderHelperEditor::DebugVertex(uint32 InVertexIndex, uint32 InInstanceIndex)
	{
		SShaderEditorBox* ShaderEditor = GetShaderEditor(GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem));
		Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
		Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());

		InvocationState Invocation = GetDebuggaleObject()->GetInvocationState(DebugItem::Vertex);

		GApp->EnqueueBusyTask([=, this](TFunction<void()> Done) {
			FNotificationInfo Info(LOCALIZATION("StartDebuggerTip"));
			Info.Image = FAppStyle::Get().GetBrush("NoBrush");
			Info.bFireAndForget = false;
			Info.FadeInDuration = 0.0f;
			Info.FadeOutDuration = 0.0f;
			auto Notification = FSlateNotificationManager::Get().AddNotification(Info);
			Notification->SetCompletionState(SNotificationItem::CS_Pending);
			Async(EAsyncExecution::Thread, [=, this]() {
				try
				{
					Debugger.DebugVertex(InVertexIndex, InInstanceIndex, Invocation);
				}
				catch (const std::runtime_error& e)
				{
					AsyncTask(ENamedThreads::GameThread, [=, this] {
						Notification->Fadeout();
						Done();
						FText FailureInfo = LOCALIZATION("DebugFailure");
						SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo.ToString(), UTF8_TO_TCHAR(e.what()));
						MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GetMainWindow(), FailureInfo);
						EndDebugging();
					});
					return;
				}

				AsyncTask(ENamedThreads::GameThread, [=, this] {
					Notification->Fadeout();
					Done();
					Continue();
				});
			});
		});
	}

	void ShaderHelperEditor::SwitchDebugThread(const Vector3u& InLocalInvocationId)
	{
		if (Debugger.SwitchDebugThread(InLocalInvocationId))
		{
			Debugger.ShowDebuggerResult();
			FString StopFile = Debugger.GetStopLocation().File;
			ShaderAsset* MainShader = GetDebuggaleObject()->GetShaderAsset(CurrentDebugItem);
			AssetPtr<ShaderAsset> StopShader = MainShader->FindIncludeAsset(StopFile);
			SShaderEditorBox* ShaderEditor = StopShader ? GetShaderEditor(StopShader) : GetShaderEditor(MainShader);
			if (StopShader)
			{
				AssetOp::OpenAsset(StopShader);
			}
			ShaderEditor->JumpTo(ShaderEditor->GetLineIndex(Debugger.GetStopLocation().LineNumber));
			ShaderEditor->UnFold(Debugger.GetStopLocation().LineNumber);
		}
	}

	bool ShaderHelperEditor::IsFinalizedForCurrentItem() const
	{
		if (!IsDebugging)
		{
			return false;
		}
		switch (CurrentDebugItem)
		{
		case DebugItem::Vertex:
			return VertexDebuggerViewport->FinalizedVertex();
		case DebugItem::Pixel:
			return FragmentDebuggerViewport->FinalizedPixel();
		case DebugItem::Compute:
			return ComputeDebuggerViewport->FinalizedThread();
		default:
			return false;
		}
	}

	void ShaderHelperEditor::StartDebugging(bool GlobalValidation)
	{
		NormalizeCurrentDebugItem();
		DebuggableObject* Debuggable = GetDebuggaleObject();
		if (!Debuggable || !Debuggable->GetSupportedDebugItems().Contains(CurrentDebugItem))
		{
			return;
		}

		if (CurrentDebugItem == DebugItem::Vertex)
		{
			DebugTargetInfo DebugTarget = Debuggable->OnStartDebugging(CurrentDebugItem);
			SShaderEditorBox* ShaderEditor = GetShaderEditor(Debuggable->GetShaderAsset(DebugItem::Vertex));
			Debugger.SetShaderAsset(ShaderEditor->GetShaderAsset());
			Debugger.SetShaderSource(ShaderEditor->GetCurrentShaderSource());

			InvocationState Invocation = Debuggable->GetInvocationState(DebugItem::Vertex);
			try
			{
				TArray<Vector4f> ClipPositions = Debugger.CaptureVertex(Invocation);
				IsDebugging = true;
				const auto& VState = std::get<VertexState>(Invocation);
				VertexDebuggerViewport->SetDebugData(ClipPositions, VState.Indices, VState.VertexCount, VState.InstanceCount, VState.ClipToWorld, VState.DebugCamera, GlobalValidation, MoveTemp(DebugTarget.AssertedVertices));
			}
			catch (const std::runtime_error& e)
			{
				Debuggable->OnEndDebuggging();
				Debugger.Reset();
				FText FailureInfo = LOCALIZATION("DebugFailure");
				SH_LOG(LogDebugger, Error, TEXT("%s:\n\n%s"), *FailureInfo.ToString(), UTF8_TO_TCHAR(e.what()));
				MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GetMainWindow(), FailureInfo);
			}
		}
		else if (CurrentDebugItem == DebugItem::Compute)
		{
			DebugTargetInfo DebugTarget = Debuggable->OnStartDebugging(CurrentDebugItem);
			IsDebugging = true;
			const auto& CState = std::get<ComputeState>(Debuggable->GetInvocationState(DebugItem::Compute));
			ComputeDebuggerViewport->SetComputeDebugInfo(CState.ThreadGroupCount, CState.ThreadGroupSize, GlobalValidation, MoveTemp(DebugTarget.AssertedThreads));
		}
		else if (CurrentDebugItem == DebugItem::Pixel)
		{
			DebugTargetInfo DebugTarget = Debuggable->OnStartDebugging(CurrentDebugItem);
			DebugTarget.Normalize();
			if (DebugTarget.Tex)
			{
				IsDebugging = true;
				FragmentDebuggerViewport->SetDebugTarget(DebugTarget, GlobalValidation);
			}
		}
		
	}

	FToolBarBuilder ShaderHelperEditor::CreateToolBarBuilder()
	{
		FToolBarBuilder ToolBarBuilder(UICommandList, FMultiBoxCustomization::None, nullptr);
		ToolBarBuilder.SetStyle(&FShaderHelperStyle::Get(), FName("Toolbar.ShaderHelper"));
		auto MakeCenteredToolBarWidget = [](const TSharedRef<SWidget>& InWidget)
		{
			return SNew(SBox)
				.MinDesiredHeight(25.0f)
				.VAlign(VAlign_Center)
				[
					InWidget
				];
		};
		ToolBarBuilder.AddToolBarButton(
			CodeEditorCommands::Get().GoBack,
			NAME_None,
			FText::GetEmpty(), LOCALIZATION("GoBack"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.ArrowLeft")
		);
		ToolBarBuilder.AddToolBarButton(
			CodeEditorCommands::Get().GoForward,
			NAME_None,
			FText::GetEmpty(), LOCALIZATION("GoForward"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.ArrowRight")
		);
		auto MakeDebugItemMenuContent = [this]() {
			FMenuBuilder MenuBuilder(true, nullptr);
			for (DebugItem Item : GetDebuggaleObject()->GetSupportedDebugItems())
			{
				MenuBuilder.AddMenuEntry(
					FText::FromStringTable(TEXT("Localization"), ANSI_TO_TCHAR(magic_enum::enum_name(Item).data())),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this, Item] { SetCurrentDebugItem(Item); }),
						FCanExecuteAction::CreateLambda([this] { return !IsDebugging; }),
						FIsActionChecked::CreateLambda([this, Item] { return Item == CurrentDebugItem; })
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
				);
			}
			return MenuBuilder.MakeWidget();
		};
		FToolBarBuilder DebuggerToolBarBuilder(UICommandList, FMultiBoxCustomization::None, nullptr);
		DebuggerToolBarBuilder.SetStyle(&FShaderHelperStyle::Get(), FName("Toolbar.ShaderHelper"));
		
		DebuggerToolBarBuilder.AddToolBarWidget(
			SNew(SShSplitButton)
			.CommandList(UICommandList)
			.Command(CodeEditorCommands::Get().Debug)
			.ButtonToolTipText(LOCALIZATION("Debug"))
			.MenuToolTipText_Lambda([this] { return FText::FromString(ANSI_TO_TCHAR(magic_enum::enum_name(CurrentDebugItem).data())); })
			.IsMenuEnabled_Lambda([this] { return !IsDebugging && GetDebuggaleObject() != nullptr; })
			.OnGetMenuContent_Lambda(MakeDebugItemMenuContent)
			[
				SNew(SImage)
				.Image_Lambda([this] {
					return IsDebugging
						? FShaderHelperStyle::Get().GetBrush("Icons.Pause")
						: FShaderHelperStyle::Get().GetBrush("Icons.Bug");
				})
			]
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			CodeEditorCommands::Get().Continue,
			NAME_None,
			FText::GetEmpty(), LOCALIZATION("Continue"),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.ArrowBoldRight")
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			CodeEditorCommands::Get().StepOver,
			NAME_None,
			FText::GetEmpty(), LOCALIZATION("StepOver"),
			FSlateIcon( FShaderHelperStyle::Get().GetStyleSetName(), "Icons.StepOver")
		);
		DebuggerToolBarBuilder.AddToolBarButton(
			CodeEditorCommands::Get().StepInto,
			NAME_None,
			FText::GetEmpty(), LOCALIZATION("StepInto"),
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
			MakeCenteredToolBarWidget(
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Border"))
				.Padding(1)
				[
					DebuggerToolBarBuilder.MakeWidget()
				]
			)
		);
		ToolBarBuilder.AddToolBarWidget(
			MakeCenteredToolBarWidget(
				SNew(SShSplitButton)
				.ButtonToolTipText(LOCALIZATION("Validation"))
				.MenuToolTipText_Lambda([this] { return FText::FromString(ANSI_TO_TCHAR(magic_enum::enum_name(CurrentDebugItem).data())); })
				.IsButtonEnabled_Lambda([this] { return !IsDebugging && GetDebuggaleObject() != nullptr; })
				.IsMenuEnabled_Lambda([this] { return !IsDebugging && GetDebuggaleObject() != nullptr; })
				.OnClicked_Lambda([this] {
					StartDebugging(true);
					return FReply::Handled();
				})
				.OnGetMenuContent_Lambda(MakeDebugItemMenuContent)
				[
					SNew(SImage).ColorAndOpacity(FStyleColors::Foreground)
					.Image(FShaderHelperStyle::Get().GetBrush("Icons.Validation"))
				]
			)
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
		ToolBarBuilder.AddToolBarWidget(
			MakeCenteredToolBarWidget(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(12, 0, 0, 0)
				[
					SNew(SShToggleButton)
						.Icon(FAppStyle::Get().GetBrush("Icons.Sphere"))
						.ToolTipText_Lambda([this] { return CurProject->bScenePreview ? LOCALIZATION("SwitchToCustomTip") : LOCALIZATION("SwitchToPreviewTip"); })
						.IsChecked_Lambda([this] { return !CurProject->bScenePreview ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { 
							CurProject->bScenePreview = (NewState == ECheckBoxState::Unchecked); 
							ForceRender();
						})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 0, 0, 0)
				[
					SNew(SSegmentedControl<int32>)
					.Value_Lambda([this]{ return static_cast<int32>(CurProject->GizmoMode); })
					.OnValueChanged_Lambda([this](int32 NewValue){ CurProject->GizmoMode = static_cast<SH::GizmoMode>(NewValue); })
					+ SSegmentedControl<int32>::Slot(0).Text(LOCALIZATION("Move"))
					+ SSegmentedControl<int32>::Slot(1).Text(LOCALIZATION("Rotate"))
					+ SSegmentedControl<int32>::Slot(2).Text(LOCALIZATION("Scale"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8, 0, 4, 0)
				[
					SNew(SSegmentedControl<int32>)
					.Value_Lambda([this]{ return static_cast<int32>(CurProject->GizmoSpace); })
					.OnValueChanged_Lambda([this](int32 NewValue){ CurProject->GizmoSpace = static_cast<SH::GizmoSpace>(NewValue); })
					+ SSegmentedControl<int32>::Slot(0).Text(LOCALIZATION("Global"))
					+ SSegmentedControl<int32>::Slot(1).Text(LOCALIZATION("Local"))
				]
			)
		);
		ToolBarBuilder.AddToolBarWidget(
			MakeCenteredToolBarWidget(
				SNew(STextBlock)
				.Margin(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
				.Visibility_Lambda([this] {
					return (IsDebugging && bShowingLinePreview) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
				.Text_Lambda([this] {
					const auto& PreviewData = Debugger.GetLinePreviewData();
					if (const auto* Info = PreviewData.Find(LinePreviewLocation))
					{
						return FText::Format(LOCALIZATION("InspectingPreview"),
							FText::FromString(Info->VarName),
							FText::FromString(LinePreviewLocation.File),
							FText::AsNumber(LinePreviewLocation.LineNumber));
					}
					return FText::GetEmpty();
				})
			)
		);
		return ToolBarBuilder;
	}

    FMenuBarBuilder ShaderHelperEditor::CreateMenuBarBuilder()
	{
		FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(UICommandList);
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

	void ShaderHelperEditor::ShowAboutWindow()
	{
		if (AboutWindow.IsValid())
		{
			AboutWindow.Pin()->BringToFront();
			return;
		}

		auto MakeVersionRow = [](const FText& Name, const FString& Version, const FString& Url) -> TSharedRef<SWidget>
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SHyperlink)
					.Text(Name)
					.OnNavigate_Lambda([Url] {
						FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(20.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Version))
				];
		};

		TSharedRef<SShWindow> NewWindow = SNew(SShWindow)
			.Title_Lambda([] {
				return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + TEXT("-") + LOCALIZATION("About").ToString());
			})
			.SizingRule(ESizingRule::Autosized)
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		AboutWindow = NewWindow;
		FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, MainWindow.ToSharedRef());
		NewWindow->SetContent(
			SNew(SBorder)
			.Padding(16.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					MakeVersionRow(LOCALIZATION("ShaderHelper"), TEXT("0.1"), TEXT("https://github.com/ShaderHelper/ShaderHelper/"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					MakeVersionRow(FText::FromString(TEXT("DirectXShaderCompiler")), TEXT("1.8.2502"), TEXT("https://github.com/ShaderHelper/DirectXShaderCompiler/"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					MakeVersionRow(FText::FromString(TEXT("glslang")), TEXT("16.1.0"), TEXT("https://github.com/ShaderHelper/glslang"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCALIZATION("Ok"))
					.OnClicked_Lambda([this]
					{
						if (AboutWindow.IsValid())
						{
							AboutWindow.Pin()->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
				]
			]
		);
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
										auto Ret = MessageDialog::Open(MessageDialog::OkNoCancel, MessageDialog::Shocked, MainWindow, LOCALIZATION("SaveAssetsTip"));
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
			MenuBuilder.BeginSection("EditUndoRedo");
			{
				MenuBuilder.AddMenuEntry(
					EditorCommands::Get().Undo,
					NAME_None,
					LOCALIZATION("Undo"),
					FText::GetEmpty(),
					FSlateIcon()
				);
				MenuBuilder.AddMenuEntry(
					EditorCommands::Get().Redo,
					NAME_None,
					LOCALIZATION("Redo"),
					FText::GetEmpty(),
					FSlateIcon()
				);
			}
			MenuBuilder.EndSection();

			MenuBuilder.AddMenuEntry(LOCALIZATION("Preferences"), FText::GetEmpty(), FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Settings"),
				FUIAction(
					FExecuteAction::CreateLambda([this] {
						if (!PreferenceWindow.IsValid())
						{
							auto NewWindow = SNew(SShWindow).Title_Lambda([this] {
								return FText::FromString(LOCALIZATION("ShaderHelper").ToString() + "-" + LOCALIZATION("Preferences").ToString());
							})
							.ClientSize({ 700, 500 });
							PreferenceWindow = NewWindow;
							FSlateApplication::Get().AddWindowAsNativeChild(NewWindow, MainWindow.ToSharedRef());
							NewWindow->SetContent(SAssignNew(PreferenceView, SPreferenceView));
						}
						else
						{
							PreferenceWindow.Pin()->BringToFront();
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
			MenuBuilder.AddMenuEntry(LOCALIZATION("Document"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([] {
						const TCHAR* DocumentUrl = Editor::GetLanguage() == SupportedLanguage::Chinese ? ChineseDocumentUrl : EnglishDocumentUrl;
						FPlatformProcess::LaunchURL(DocumentUrl, nullptr, nullptr);
					})));
			MenuBuilder.AddMenuEntry(LOCALIZATION("About"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateRaw(this, &ShaderHelperEditor::ShowAboutWindow)
				));
		}


		for (MenuEntryExt* Ext : MenuEntryExts)
		{
			if (Ext->TargetMenu == MenuName)
			{
				MenuBuilder.AddMenuEntry(FText::FromString(Ext->Label), FText::GetEmpty(), FSlateIcon(), 
					FUIAction(
						FExecuteAction::CreateLambda([Ext] { Ext->OnExecute();  }),
						FCanExecuteAction::CreateLambda([Ext] { return Ext->CanExecute(); })
					));
			}
		}

	}

	void ShaderHelperEditor::RefreshActiveUndoContext() const
	{
		TSharedPtr<SWidget> Focus = FSlateApplication::Get().GetKeyboardFocusedWidget();
		while (Focus.IsValid())
		{
			if (GraphPanel.IsValid() && Focus == GraphPanel)
			{
				ActiveUndoContext = EActiveUndoContext::Graph;
				return;
			}
			for (const auto& Pair : ShaderEditors)
			{
				if (Pair.Value.IsValid() && Focus == Pair.Value)
				{
					ActiveUndoContext = EActiveUndoContext::Code;
					ActiveShaderEditor = Pair.Value;
					return;
				}
			}
			Focus = Focus->GetParentWidget();
		}

		ActiveUndoContext = EActiveUndoContext::Scene;
	}

	bool ShaderHelperEditor::CanUndoActive() const
	{
		RefreshActiveUndoContext();
		switch (ActiveUndoContext)
		{
		case EActiveUndoContext::Graph:
			return GraphPanel.IsValid() && GraphPanel->CanUndo();
		case EActiveUndoContext::Code:
			if (auto Editor = ActiveShaderEditor.Pin())
			{
				return Editor->ShaderMultiLineEditableTextLayout && Editor->ShaderMultiLineEditableTextLayout->CanExecuteUndo();
			}
			return false;
		case EActiveUndoContext::Scene:
			return SceneView.IsValid() && SceneView->CanUndo();
		default:
			return false;
		}
	}

	bool ShaderHelperEditor::CanRedoActive() const
	{
		RefreshActiveUndoContext();
		switch (ActiveUndoContext)
		{
		case EActiveUndoContext::Graph:
			return GraphPanel.IsValid() && GraphPanel->CanRedo();
		case EActiveUndoContext::Code:
			if (auto Editor = ActiveShaderEditor.Pin())
			{
				auto* Layout = Editor->ShaderMultiLineEditableTextLayout;
				return Layout && Layout->CurrentUndoLevel != INDEX_NONE && Layout->UndoStates.Num() > 0;
			}
			return false;
		case EActiveUndoContext::Scene:
			return SceneView.IsValid() && SceneView->CanRedo();
		default:
			return false;
		}
	}

	void ShaderHelperEditor::DoUndoActive()
	{
		switch (ActiveUndoContext)
		{
		case EActiveUndoContext::Graph:
			if (GraphPanel.IsValid()) GraphPanel->Undo();
			break;
		case EActiveUndoContext::Code:
			if (auto Editor = ActiveShaderEditor.Pin())
			{
				if (Editor->ShaderMultiLineEditableTextLayout) Editor->ShaderMultiLineEditableTextLayout->Undo();
			}
			break;
		case EActiveUndoContext::Scene:
			if (SceneView.IsValid()) SceneView->Undo();
			break;
		default:
			break;
		}
	}

	void ShaderHelperEditor::DoRedoActive()
	{
		switch (ActiveUndoContext)
		{
		case EActiveUndoContext::Graph:
			if (GraphPanel.IsValid()) GraphPanel->Redo();
			break;
		case EActiveUndoContext::Code:
			if (auto Editor = ActiveShaderEditor.Pin())
			{
				if (Editor->ShaderMultiLineEditableTextLayout) Editor->ShaderMultiLineEditableTextLayout->Redo();
			}
			break;
		case EActiveUndoContext::Scene:
			if (SceneView.IsValid()) SceneView->Redo();
			break;
		default:
			break;
		}
	}

}
