#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/ShaderPass.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"

namespace SH 
{
	class ShaderHelperEditor : public FRAMEWORK::Editor
	{
	public:
		struct WindowLayoutConfigInfo
		{
			FVector2D Position;
			FVector2D WindowSize;
			TSharedPtr<FTabManager::FLayout> TabLayout;
		};

		ShaderHelperEditor(const FRAMEWORK::Vector2f& InWindowSize, ShRenderer* InRenderer);
		~ShaderHelperEditor();
    public:
        FTabManager* GetCodeTabManager() const { return CodeTabManager.Get(); }
        
    public:
        void Update(double DeltaTime) override;
		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void LoadEditorState(const FString& InFile);
		void SaveEditorState();
		void OnViewportResize(const FRAMEWORK::Vector2f& InSize);
        
        void OpenShaderPassTab(FRAMEWORK::AssetPtr<ShaderPass> InShaderPass);
        void TryRestoreShaderPassTab(FRAMEWORK::AssetPtr<ShaderPass> InShaderPass);
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
        TSharedRef<SDockTab> SpawnShaderPassTab(const FSpawnTabArgs& Args);
        TSharedRef<SWidget> SpawnShaderPassPath(const FString& InShaderPassPath);
        FMenuBarBuilder CreateMenuBarBuilder();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		void InitEditorUI();
		
	private:
		ShRenderer* Renderer;
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
        FDelegateHandle SaveLayoutTicker;
        TSharedPtr<FRAMEWORK::SAssetBrowser> AssetBrowser;
        
        TSharedPtr<SDockTab> CodeTab;
        TSharedPtr<FTabManager> CodeTabManager;
        TWeakPtr<class SDockingTabStack> ShaderPassTabStackInsertPoint;
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

            TSharedPtr<FRAMEWORK::AssetBrowserPersistentState> AssetBrowserState = MakeShared<FRAMEWORK::AssetBrowserPersistentState>();
            TSharedPtr<FTabManager::FLayout> CodeTabLayout;
            TMap<FRAMEWORK::AssetPtr<ShaderPass>, TSharedPtr<SDockTab>> OpenedShaderPasses;
		} CurEditorState;
        
        //used to restore the ShaderEditorBox when the shaderpass asset path is changed.
        TMap<FRAMEWORK::AssetPtr<ShaderPass>, TSharedPtr<class SShaderEditorBox>> PendingShaderPasseTabs;
		FString EditorStateSaveFileName;
	};

}

