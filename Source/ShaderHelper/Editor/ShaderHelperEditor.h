#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/ShaderPass.h"
#include "AssetManager/AssetManager.h"

namespace SH 
{
	class ShaderHelperEditor : public FRAMEWORK::Editor
	{
	public:
		DECLARE_DELEGATE_OneParam(OnEditorWindowClosed, bool)

		struct WindowLayoutConfigInfo
		{
			FVector2D Position;
			FVector2D WindowSize;
			TSharedPtr<FTabManager::FLayout> TabLayout;
		};

		ShaderHelperEditor(const FRAMEWORK::Vector2f& InWindowSize, ShRenderer* InRenderer);
		~ShaderHelperEditor();

		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void LoadEditorState(const FString& InFile);
		void SaveEditorState();
		void OnViewportResize(const FRAMEWORK::Vector2f& InSize);
        
        void OpenShaderPassTab(FRAMEWORK::AssetPtr<ShaderPass> InShaderPass);
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
        TSharedRef<SDockTab> SpawnShaderPassTab(const FSpawnTabArgs& Args);
        FMenuBarBuilder CreateMenuBarBuilder();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		void InitEditorUI();

	public:
		OnEditorWindowClosed OnWindowClosed;
		
	private:
		ShRenderer* Renderer;
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
        FDelegateHandle SaveLayoutTicker;
        
        TSharedPtr<SDockTab> CodeTab;
        TSharedPtr<FTabManager> CodeTabManager;
        TWeakPtr<class SDockingTabStack> LastActivedShaderPassTabStack;
        TSharedPtr<class SDockingArea> CodeTabMainArea;
        
		TSharedPtr<SWindow> Window;
		TSharedPtr<FRAMEWORK::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

        TSharedPtr<SVerticalBox> WindowContentBox;
		TWeakPtr<SBox> PropertyViewBox;

		struct EditorState
		{
			void InitFromJson(const TSharedPtr<FJsonObject>& InJson);
			TSharedRef<FJsonObject> ToJson() const;

			float AssetViewSize = 60;
			FString SelectedDirectory;
			TArray<FString> DirectoriesToExpand;
            TSharedPtr<FTabManager::FLayout> CodeTabLayout;
            TMap<FRAMEWORK::AssetPtr<ShaderPass>, TSharedPtr<SDockTab>> OpenedShaderPasses;
		} CurEditorState;
		FString EditorStateSaveFileName;
	};

}

