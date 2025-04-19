#pragma once
#include <Widgets/SViewport.h>
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/StShader.h"
#include "AssetObject/Graph.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include "Renderer/RenderComponent.h"
#include "Renderer/Renderer.h"
#include "ProjectManager/ShProjectManager.h"
#include "UI/Widgets/Property/PropertyView/SPropertyView.h"

namespace SH 
{
	class ShaderHelperEditor : public FW::Editor
	{
	public:
		struct WindowLayoutConfigInfo
		{
			FVector2D Position;
			FVector2D WindowSize;
			TSharedPtr<FTabManager::FLayout> TabLayout;
		};

		ShaderHelperEditor(const FW::Vector2f& InWindowSize, ShRenderer* InRenderer);
		~ShaderHelperEditor();
    public:
        FTabManager* GetCodeTabManager() const { return CodeTabManager.Get(); }
		TSharedPtr<SWindow> GetMainWindow() const override { return Window; }
		FW::PreviewViewPort* GetViewPort() const { return ViewPort.Get(); }
        
    public:
		void InitEditorUI();

		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
        
        void OpenStShaderTab(FW::AssetPtr<StShader> InStShader);
		
		void OpenGraph(FW::AssetPtr<FW::Graph> InGraphData, TSharedPtr<FW::RenderComponent> InGraphRenderComp);
        void RefreshProperty();
        void ShowProperty(FW::ShObject* InObjectData);
        void UpdateStShaderPath(const FString& InStShaderPath);

	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
        TSharedRef<SDockTab> SpawnStShaderTab(const FSpawnTabArgs& Args);
        TSharedRef<SWidget> SpawnStShaderPath(const FString& InStShaderPath);
        FMenuBarBuilder CreateMenuBarBuilder();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		
	private:
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
        FDelegateHandle SaveLayoutTicker;
        TSharedPtr<FW::SAssetBrowser> AssetBrowser;
		TMap<FW::AssetPtr<StShader>, TSharedPtr<SShaderEditorBox>> ShaderEditors;

        TSharedPtr<SDockTab> CodeTab;
        TSharedPtr<FTabManager> CodeTabManager;
        TWeakPtr<class SDockingTabStack> StShaderTabStackInsertPoint;
        TSharedPtr<class SDockingArea> CodeTabMainArea;
        
		TSharedPtr<SWindow> Window;
		TSharedPtr<FW::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

        TSharedPtr<SVerticalBox> WindowContentBox;
		TSharedPtr<FW::SPropertyView> PropertyView;
        FW::ShObject* CurPropertyObject = nullptr;
        
		TSharedPtr<ShProject> CurProject;

		TSharedPtr<FW::SGraphPanel> GraphPanel;

		ShRenderer* Renderer;
		TSharedPtr<FW::RenderComponent> GraphRenderComp;
        
        TMap<FW::AssetPtr<StShader>, TSharedPtr<SScrollBox>> StShaderPathBoxMap;
	};

}

