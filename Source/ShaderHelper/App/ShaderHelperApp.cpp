#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/FShaderHelperStyle.h"
#include "UI/SShaderHelperWindow.h"

namespace SH {
	ShaderHelperApp::ShaderHelperApp(const TCHAR* CommandLine)
		:App(CommandLine)
	{

	}

	void ShaderHelperApp::Init()
	{
		App::Init();
		FShaderHelperStyle::Init();
	}

	void ShaderHelperApp::PostInit()
	{
		SAssignNew(AppWindow, SShaderHelperWindow);
		App::PostInit();
	}


	void ShaderHelperApp::ShutDown()
	{
		App::ShutDown();
		FShaderHelperStyle::ShutDown();
	}


	void ShaderHelperApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

}