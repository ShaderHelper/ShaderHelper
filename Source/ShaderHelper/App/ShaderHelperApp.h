#pragma once
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/ProjectLauncher.h"
#include "ProjectManager/ShProjectManager.h"

namespace SH {
	
	class ShaderHelperApp : public FRAMEWORK::App
	{
	public:
		ShaderHelperApp(const FRAMEWORK::Vector2D& InClientSize, const TCHAR* CommandLine);
		~ShaderHelperApp();

	private:
		void Update(double DeltaTime) override;
		void Render() override;

	public:
		TSharedPtr<FRAMEWORK::ProjectLauncher<ShProject>> Launcher;
	};
	
}


