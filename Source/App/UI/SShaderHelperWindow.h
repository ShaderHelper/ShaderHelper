#pragma once
#include "CommonHeaderForUE.h"

namespace SH {

	class SShaderHelperWindow :
		public SWindow
	{
	public:
		SLATE_BEGIN_ARGS(SShaderHelperWindow) {}
		SLATE_END_ARGS()

		SShaderHelperWindow();
		~SShaderHelperWindow();

		void Construct(const FArguments& InArgs);

		TSharedRef<SWidget> CreateMenuBar();
		void FillFileMenu();
		void FillConfigMenu();
		void FillWindowMenu();

	};

}

