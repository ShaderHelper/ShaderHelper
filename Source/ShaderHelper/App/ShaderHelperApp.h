#pragma once
#include "App/App.h"
#include "PreviewViewPort.h"

namespace SH {
	
	class ShaderHelperApp : public App
	{
	public:
		using App::App;
		
	private:
		void Init() override;

		void InitEditorUI();
		void ReInitEditorUi();
		void InitLauncherUI();

		void ShutDown() override;
		void PostInit() override;
		void Update(double DeltaTime) override;
		
	private:
		bool IsReInitEditorUI = false;
		TSharedPtr<PreviewViewPort> ViewPort;
	};
	
}


