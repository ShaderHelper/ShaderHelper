#pragma once
#include <Widgets/SViewport.h>

namespace SH {

	class SShaderHelperWindow :
		public SWindow
	{
	public:
		SLATE_BEGIN_ARGS(SShaderHelperWindow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetViewPortInterface(TSharedRef<ISlateViewport> InSlateViewportInterface) {
			Viewport->SetViewportInterface(MoveTemp(InSlateViewportInterface));
		}
		
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
		TSharedRef<SWidget> CreateMenuBar();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);

	private:
		TSharedPtr<FTabManager> TabManager;
		TSharedPtr<SViewport> Viewport;
	};

}

