#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/SShaderHelperWindow.h"

namespace SH {

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