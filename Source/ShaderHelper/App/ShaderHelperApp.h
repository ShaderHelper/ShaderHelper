#pragma once
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/ProjectLauncher.h"
#include "ProjectManager/ShProjectManager.h"

namespace SH {
	
	class ShaderHelperApp : public FW::App
	{
	public:
		ShaderHelperApp(const FW::Vector2D& InClientSize, const TCHAR* CommandLine);
		~ShaderHelperApp();

	public:
		void Init() override;

	private:
		void Update(float DeltaTime) override;
		void Render() override;

	public:
		TSharedPtr<FW::ProjectLauncher<ShProject>> Launcher;
	};
	
}


