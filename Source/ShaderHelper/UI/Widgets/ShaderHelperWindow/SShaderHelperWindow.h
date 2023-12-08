#pragma once
#include <Widgets/SViewport.h>
#include "Renderer/ShRenderer.h"
namespace SH 
{
	class SShaderHelperWindow :
		public SWindow
	{
	public:
		struct WindowLayoutConfigInfo
		{
			FVector2D Position;
			FVector2D WindowSize;
			TSharedPtr<FTabManager::FLayout> TabLayout;
		};

		~SShaderHelperWindow();

		SLATE_BEGIN_ARGS(SShaderHelperWindow) 
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(ShRenderer*, Renderer)
			SLATE_ARGUMENT(FVector2D, WindowSize)
			SLATE_EVENT(FSimpleDelegate, OnResetWindowLayout)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetViewPortInterface(TSharedRef<ISlateViewport> InSlateViewportInterface) {
			Viewport->SetViewportInterface(MoveTemp(InSlateViewportInterface));
		}
		void ResetWindowLayout();
		WindowLayoutConfigInfo LoadWindowLayout(const FString& InWindowLayoutConfigFileName);
		void SaveWindowLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
		TSharedRef<SWidget> CreateMenuBar();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
		
	private:
		ShRenderer* Renderer;
		bool bSaveWindowLayout = true;
		TSharedPtr<FTabManager::FLayout> DefaultTabLayout;
		TSharedPtr<SVerticalBox> LayoutBox;
		TSharedPtr<FTabManager> TabManager;
		TSharedPtr<SViewport> Viewport;
		FVector2D WindowSize;

		TSharedPtr<class SShaderEditorBox> ShaderEditor;

		FSimpleDelegate OnResetWindowLayout;
	};

}

