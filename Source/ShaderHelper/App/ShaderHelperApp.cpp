#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "Renderer/ShRenderer.h"

using namespace FRAMEWORK;

namespace SH {

	ShaderHelperApp::ShaderHelperApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
        AppRenderer = MakeUnique<ShRenderer>();
        AppEditor = MakeUnique<ShaderHelperEditor>(AppClientSize, static_cast<ShRenderer*>(GetRenderer()));
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
