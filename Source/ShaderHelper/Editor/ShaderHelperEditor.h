#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
#include "Editor/Editor.h"
#include "PreviewViewPort.h"

namespace SH 
{
	class ShaderHelperEditor : public Editor
	{
	public:
		struct WindowLayoutConfigInfo
		{
			FVector2D Position;
			FVector2D WindowSize;
			TSharedPtr<FTabManager::FLayout> TabLayout;
		};

		ShaderHelperEditor(const Vector2f& InWindowSize, ShRenderer* InRenderer);
		~ShaderHelperEditor();

		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void OnViewportResize(const Vector2f& InSize);;
		void Update(double DeltaTime);
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
		TSharedRef<SWidget> CreateMenuBar();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		void InitEditorUI();

	public:
		FSimpleDelegate OnResetWindowLayout;
		FSimpleDelegate OnWindowClosed;
		
	private:
		ShRenderer* Renderer;
		bool bSaveWindowLayout = true;
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SDockTab> TabManagerTab;
		TSharedPtr<FTabManager> TabManager;
		TSharedPtr<SWindow> Window;
		TSharedPtr<PreviewViewPort> ViewPort;
		FVector2D WindowSize;

		TWeakPtr<SBox> PropertyViewBox;
	};

}

