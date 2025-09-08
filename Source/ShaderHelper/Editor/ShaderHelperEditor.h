#pragma once
#include <Widgets/SViewport.h>
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/ShaderAsset.h"
#include "AssetObject/Graph.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include "Renderer/RenderComponent.h"
#include "Renderer/Renderer.h"
#include "ProjectManager/ShProjectManager.h"
#include "UI/Widgets/Log/SOutputLog.h"
#include "UI/Widgets/Property/PropertyView/SPropertyView.h"
#include "UI/Widgets/Debugger/SDebuggerViewport.h"
#include "UI/Widgets/Debugger/SDebuggerVariableView.h"
#include "UI/Widgets/Debugger/SDebuggerCallStackView.h"
#include "UI/Widgets/Debugger/SDebuggerWatchView.h"
#include "UI/Widgets/Preference/SPreferenceView.h"
#include "AssetObject/DebuggableObject.h"
#include "UI/Widgets/Misc/SShWindow.h"

namespace SH
{

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";

	const FName CodeTabId = "Code";
	const FName InitialInsertPointTabId = "CodeInsertPoint";

	const FName AssetTabId = "Asset";
	const FName GraphTabId = "Graph";
	const FName LogTabId = "Log";

	const FName CallStackTabId = "CallStack";
	const FName LocalTabId = "Local";
	const FName GlobalTabId = "Global";
	const FName WatchTabId = "Watch";

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
		ShRenderer* GetRenderer() const { return Renderer;}
        FTabManager* GetCodeTabManager() const { return CodeTabManager.Get(); }
		FTabManager* GetTabManager() const { return TabManager.Get(); }
		TSharedPtr<SWindow> GetMainWindow() const override { return MainWindow; }
		FW::PreviewViewPort* GetViewPort() const { return ViewPort.Get(); }
		FW::ShObject* GetCurPropertyObject() const { return CurPropertyObject; }
		TSharedPtr<FUICommandList> GetUICommandList() const { return UICommandList; }
		SDebuggerVariableView* GetDebuggerLocalVariableView() const { return DebuggerLocalVariableView.Get(); }
		SDebuggerVariableView* GetDebuggerGlobalVariableView() const { return DebuggerGlobalVariableView.Get(); }
		SDebuggerCallStackView* GetDebuggerCallStackView() const { return DebuggerCallStackView.Get(); }
		SDebuggerWatchView* GetDebuggerWatchView() const { return DebuggerWatchView.Get(); }
		FW::SGraphPanel* GetGraphPanel() const { return GraphPanel.Get(); }
		void InvokeDebuggerTabs();
		void CloseDebuggerTabs();
        
    public:
		void InitEditorUI();

		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void ForceRender();
        void OpenShaderTab(FW::AssetPtr<ShaderAsset> InShader);
		
		void OpenGraph(FW::AssetPtr<FW::Graph> InGraphData, TSharedPtr<FW::RenderComponent> InGraphRenderComp);
        void RefreshProperty();
        void ShowProperty(FW::ShObject* InObjectData);
        void UpdateShaderPath(const FString& InShaderPath);
		
		DebuggableObject* GetDebuggaleObject() const { return CurDebuggableObject; }
		void SetDebuggableObject(DebuggableObject* InObject) { CurDebuggableObject = InObject; }
		void EndDebugging();
		void StartDebugging();
		SShaderEditorBox* GetShaderEditor(FW::AssetPtr<ShaderAsset> InShader) {
			if(!ShaderEditors.Contains(InShader))
			{
				return nullptr;
			}
			return ShaderEditors[InShader].Get();
		}

	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
        TSharedRef<SDockTab> SpawnShaderTab(const FSpawnTabArgs& Args);
        TSharedRef<SWidget> SpawnShaderPath(const FString& InShaderPath);
		FToolBarBuilder CreateToolBarBuilder();
        FMenuBarBuilder CreateMenuBarBuilder();
		void CreateInternalWidgets();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		
	private:
		TSharedPtr<FUICommandList> UICommandList;
		
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
        FDelegateHandle SaveLayoutTicker;
        TSharedPtr<FW::SAssetBrowser> AssetBrowser;
		TMap<FW::AssetPtr<ShaderAsset>, TSharedPtr<SShaderEditorBox>> ShaderEditors;

        TSharedPtr<SDockTab> CodeTab;
        TSharedPtr<FTabManager> CodeTabManager;
        TWeakPtr<class SDockingTabStack> ShaderTabStackInsertPoint;
        TSharedPtr<class SDockingArea> CodeTabMainArea;
        
		TSharedPtr<FW::SShWindow> MainWindow;
		TWeakPtr<FW::SShWindow> PreferenceWindow;
		TSharedPtr<SPreferenceView> PreferenceView;
		TSharedPtr<SViewport> ViewportWidget;
		TSharedPtr<FW::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

        TSharedPtr<SVerticalBox> WindowContentBox;
		TSharedPtr<FW::SPropertyView> PropertyView;
        FW::ShObject* CurPropertyObject = nullptr;
        
		TSharedPtr<ShProject> CurProject;

		TSharedPtr<FW::SOutputLog> OutputLog;
		TSharedPtr<FW::SGraphPanel> GraphPanel;

		ShRenderer* Renderer;
		TSharedPtr<FW::RenderComponent> GraphRenderComp;
        
        TMap<FW::AssetPtr<ShaderAsset>, TSharedPtr<SScrollBox>> ShaderPathBoxMap;
		
		TSharedPtr<SDebuggerVariableView> DebuggerLocalVariableView;
		TSharedPtr<SDebuggerVariableView> DebuggerGlobalVariableView;
		TSharedPtr<SDebuggerCallStackView> DebuggerCallStackView;
		TSharedPtr<SDebuggerWatchView> DebuggerWatchView;
		TSharedPtr<SDebuggerViewport> DebuggerViewport;
		DebuggableObject* CurDebuggableObject = nullptr;
		bool IsDebugging{};
	};

}

