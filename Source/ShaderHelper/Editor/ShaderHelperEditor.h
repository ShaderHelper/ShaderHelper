#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/StShader.h"
#include "AssetObject/Graph.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Graph/SGraphPanel.h"

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
		void OnViewportResize(const FRAMEWORK::Vector2f& InSize);
        
        void OpenStShaderTab(FRAMEWORK::AssetPtr<StShader> InStShader);
        void TryRestoreStShaderTab(FRAMEWORK::AssetPtr<StShader> InStShader);
		
		void OpenGraph(FRAMEWORK::AssetPtr<FRAMEWORK::Graph> InGraphData);

	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
        TSharedRef<SDockTab> SpawnStShaderTab(const FSpawnTabArgs& Args);
        TSharedRef<SWidget> SpawnStShaderPath(const FString& InStShaderPath);
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
        TWeakPtr<class SDockingTabStack> StShaderTabStackInsertPoint;
        TSharedPtr<class SDockingArea> CodeTabMainArea;
        
		TSharedPtr<SWindow> Window;
		TSharedPtr<FRAMEWORK::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

        TSharedPtr<SVerticalBox> WindowContentBox;
		TWeakPtr<SBox> PropertyViewBox;
        
		class ShProject* CurProject;
        //used to restore the ShaderEditorBox when the StShader asset path is changed.
        TMap<FRAMEWORK::AssetPtr<StShader>, TSharedPtr<class SShaderEditorBox>> PendingStShadereTabs;

		TSharedPtr<FRAMEWORK::SGraphPanel> GraphPanel;
	};

}

