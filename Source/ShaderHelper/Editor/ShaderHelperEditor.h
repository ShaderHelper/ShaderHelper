#pragma once
#include "App/App.h"
#include "Editor/Editor.h"
#include "AssetManager/AssetManager.h"
#include "UI/Widgets/Property/PropertyView/SPropertyView.h"
#include "UI/Widgets/SShViewport.h"
#include "Debugger/ShaderDebugger.h"

#include <Widgets/SViewport.h>
#include <Framework/Docking/TabManager.h>
#include <Framework/Text/TextLayout.h>

class FMenuBarBuilder;
class FMenuBuilder;
class FSpawnTabArgs;
class FToolBarBuilder;
class FUICommandList;
class SScrollBox;

namespace FW
{
	class Graph;
	class PreviewViewPort;
	class RenderComponent;
	class SAssetBrowser;
	class SGraphPanel;
	class SOutputLog;
	class SShWindow;
}

namespace SH
{
	class DebuggableObject;
	enum class DebugItem;
	enum class GizmoMode : int32;
	enum class GizmoSpace : int32;
	class SDebuggerCallStackView;
	class SDebuggerVariableView;
	class SDebuggerWatchView;
	class SFragmentDebuggerViewport;
	class SComputeDebuggerViewport;
	class SPreferenceView;
	class SSceneView;
	class SShaderEditorBox;
	class SVertexDebuggerViewport;
	class ShaderAsset;
	class ShProject;
	class ShRenderer;

	const FName PreviewTabId = "Preview";
	const FName PropretyTabId = "Propety";
	const FName SceneTabId = "Scene";

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
		FW::RenderComponent* GetGraphRenderComp() const { return GraphRenderComp.Get(); }
        FTabManager* GetCodeTabManager() const { return CodeTabManager.Get(); }
		FTabManager* GetTabManager() const { return TabManager.Get(); }
		TSharedPtr<SWindow> GetMainWindow() const override;
		TWeakPtr<SWindow> GetPreferenceWindow() const;
		FW::PreviewViewPort* GetViewPort() const { return ViewPort.Get(); }
		FW::ShObject* GetCurPropertyObject() const { 
			return CurPropertyObject.IsValid() ? CurPropertyObject.Get() : nullptr;
		}
		TSharedPtr<FUICommandList> GetUICommandList() const { return UICommandList; }
		SDebuggerVariableView* GetDebuggerLocalVariableView() const { return DebuggerLocalVariableView.Get(); }
		SDebuggerVariableView* GetDebuggerGlobalVariableView() const { return DebuggerGlobalVariableView.Get(); }
		SDebuggerCallStackView* GetDebuggerCallStackView() const { return DebuggerCallStackView.Get(); }
		SDebuggerWatchView* GetDebuggerWatchView() const { return DebuggerWatchView.Get(); }
		SComputeDebuggerViewport* GetComputeDebuggerViewport() const { return ComputeDebuggerViewport.Get(); }
		TSharedPtr<SWindow> GetShaderEditorTipWindow() const { return ShaderEditorTipWindow; }
		FW::SGraphPanel* GetGraphPanel() const { return GraphPanel.Get(); }
		FW::SAssetBrowser* GetAssetBrowser() const { return AssetBrowser.Get(); }
		SSceneView* GetSceneView() const { return SceneView.Get(); }
		GizmoMode GetGizmoMode() const;
		void SetGizmoMode(GizmoMode Mode);
		GizmoSpace GetGizmoSpace() const;
		bool IsScenePreview() const;
		ShaderDebugger& GetDebugger() { return Debugger; }
		void InvokeDebuggerTabs();
		void CloseDebuggerTabs();
        
    public:
		void InitEditorUI();

		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void ForceRender();
        void OpenShaderTab(FW::AssetPtr<ShaderAsset> InShader);
		
		bool OpenGraph(FW::AssetPtr<FW::Graph> InGraphData);
		void SetGraphRenderComp(TSharedPtr<FW::RenderComponent> InGraphRenderComp);
        void RefreshProperty(bool bClear = false) override;
		void ShowProperty(FW::ShObject* InObjectData) override;
        void UpdateShaderPath(const FString& InShaderPath);
		bool IsPropertyLocked() const;
		void AddNavigationInfo(FW::AssetPtr<ShaderAsset> InShader, const FTextLocation& InLocation);
		
		DebuggableObject* GetDebuggaleObject() const;
		bool IsInDebugging() const { return IsDebugging; }
		void SetDebuggableObject(FW::ShObject* InObject);
		DebugItem GetCurrentDebugItem() const { return CurrentDebugItem; }
		void SetCurrentDebugItem(DebugItem InItem);
		void EndDebugging();
		//If globalvalidation is true, the validation error location will be automatically selected.
		void StartDebugging(bool GlobalValidation = false);
		void Continue(StepMode Mode = StepMode::Continue);
		std::optional<FW::Vector2u> ValidatePixel(const InvocationState& InState);
		std::optional<TPair<FW::Vector3u, FW::Vector3u>> ValidateCompute(const InvocationState& InState);
		void DebugPixel(const FW::Vector2u& InPixelCoord, const InvocationState& InState);
		void DebugCompute(const FW::Vector3u& InWorkGroupId, const FW::Vector3u& InLocalInvocationId, const InvocationState& InState);
		void DebugVertex(uint32 InVertexIndex, uint32 InInstanceIndex);
		void SwitchDebugThread(const FW::Vector3u& InLocalInvocationId);
		bool IsFinalizedForCurrentItem() const;
		void ShowLinePreview(const DebuggerLocation& Loc);
		void DismissLinePreview();
		bool IsShowingLinePreview() const { return bShowingLinePreview; }
		const DebuggerLocation& GetLinePreviewLocation() const { return LinePreviewLocation; }

		TArray<TSharedPtr<SShaderEditorBox>> GetShaderEditors() const { 
			TArray<TSharedPtr<SShaderEditorBox>> Ret;
			ShaderEditors.GenerateValueArray(Ret);
			return Ret;
		}
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
		void CreateBuiltinAssets();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		void ShowAboutWindow();
		void NormalizeCurrentDebugItem();

		enum class EActiveUndoContext { None, Graph, Code, Scene };
		void RefreshActiveUndoContext() const;
		bool CanUndoActive() const;
		bool CanRedoActive() const;
		void DoUndoActive();
		void DoRedoActive();
		
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
		TWeakPtr<FW::SShWindow> AboutWindow;
		TSharedPtr<SPreferenceView> PreferenceView;
		TSharedPtr<SShViewport> ViewportWidget;
		TSharedPtr<FW::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

        TSharedPtr<SVerticalBox> WindowContentBox;
		TSharedPtr<FW::SPropertyView> PropertyView;
        FW::ObserverObjectPtr<FW::ShObject> CurPropertyObject;
        
		TSharedPtr<ShProject> CurProject;

		TSharedPtr<FW::SOutputLog> OutputLog;
		TSharedPtr<FW::SGraphPanel> GraphPanel;

		TSharedPtr<SSceneView> SceneView;

		ShRenderer* Renderer;
		TSharedPtr<FW::RenderComponent> GraphRenderComp;
        
        TMap<FW::AssetPtr<ShaderAsset>, TSharedPtr<SScrollBox>> ShaderPathBoxMap;
		
		TSharedPtr<SDebuggerVariableView> DebuggerLocalVariableView;
		TSharedPtr<SDebuggerVariableView> DebuggerGlobalVariableView;
		TSharedPtr<SDebuggerCallStackView> DebuggerCallStackView;
		TSharedPtr<SDebuggerWatchView> DebuggerWatchView;
		TSharedPtr<SFragmentDebuggerViewport> FragmentDebuggerViewport;
		TSharedPtr<SComputeDebuggerViewport> ComputeDebuggerViewport;
		TSharedPtr<SVertexDebuggerViewport> VertexDebuggerViewport;
		TSharedPtr<SViewport> LinePreviewWidget;
		TSharedPtr<FW::PreviewViewPort> LinePreviewViewPort;
		bool bShowingLinePreview{};
		DebuggerLocation LinePreviewLocation;
		TSharedPtr<SWindow> ShaderEditorTipWindow;
		FW::ObserverObjectPtr<FW::ShObject> CurDebuggableObject;
		DebugItem CurrentDebugItem;
		bool IsDebugging{};
		ShaderDebugger Debugger;

		static constexpr int MaxNavigation = 233;
		int32 NavigationIndex = INDEX_NONE;
		TArray<TPair<FW::AssetPtr<ShaderAsset>, FTextLocation>> NavigationHistory;
		
		double LastForceRenderTime = 0.0;
		static constexpr double ForceRenderThrottleInterval = 1.0 / 60.0;

		mutable EActiveUndoContext ActiveUndoContext = EActiveUndoContext::None;
		mutable TWeakPtr<SShaderEditorBox> ActiveShaderEditor;
	};

}

