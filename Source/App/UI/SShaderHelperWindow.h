#pragma once
#include "CommonHeaderForUE.h"

namespace SH {

	class SShaderHelperWindow :
		public SWindow
	{
	public:
		SLATE_BEGIN_ARGS(SShaderHelperWindow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	private:
		TSharedRef<SDockTab> SpawnWindowTab(const FSpawnTabArgs& Args);
		TSharedRef<SWidget> CreateMenuBar();
		void FillMenu(FMenuBuilder& MenuBuilder, FString MenuName);
	private:
		TSharedPtr<FTabManager> TabManager;
	};

}

