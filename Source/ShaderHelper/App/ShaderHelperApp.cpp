#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "Renderer/ShRenderer.h"

using namespace FW;

namespace SH {

	ShaderHelperApp::ShaderHelperApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		AppRenderer = MakeUnique<ShRenderer>();

		FString ProjectPath;
		if (FParse::Value(CommandLine, TEXT("Project="), ProjectPath, false))
		{
			TSingleton<ShProjectManager>::Get().OpenProject(ProjectPath.Replace(TEXT("\\"), TEXT("/")));
			AppEditor = MakeUnique<ShaderHelperEditor>(AppClientSize);
		}
		else
		{
			Launcher = MakeShared<ProjectLauncher<ShProject>>([this] {
				AppEditor = MakeUnique<ShaderHelperEditor>(AppClientSize);
			}, nullptr);
		}
   
	}

	ShaderHelperApp::~ShaderHelperApp()
	{

	}

	void ShaderHelperApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void ShaderHelperApp::Render()
	{
		App::Render();
	}

}
