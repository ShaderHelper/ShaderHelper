#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
#include "Editor/Editor.h"
#include "Editor/PreviewViewPort.h"

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
		void OnViewportResize(const FRAMEWORK::Vector2f& InSize);;
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
		TSharedRef<SWidget> CreateMenuBar();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		void InitEditorUI();

	public:
		OnEditorWindowClosed OnWindowClosed;
		
	private:
		ShRenderer* Renderer;
		bool bReInitEditor = false;
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
		TSharedPtr<SWindow> Window;
		TSharedPtr<FRAMEWORK::PreviewViewPort> ViewPort;
		FVector2D WindowSize;

		TWeakPtr<SBox> PropertyViewBox;

		struct EditorState
		{
			void InitFromJson(const TSharedPtr<FJsonObject>& InJson);
			TSharedRef<FJsonObject> ToJson() const;

			FString SelectedDirectory;
			TArray<FString> DirectoriesToExpand;
		} CurEditorState;
		FString EditorStateSaveFileName;
	};

}

