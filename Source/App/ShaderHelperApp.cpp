#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/FShaderHelperStyle.h"
#include "UI/SShaderHelperWindow.h"
#include "Tests/Test.h"
#include "FrameWork/Misc/Util/UnitTest.h"

namespace SH {
#if UNIT_TEST
	static void UnitTestInit() {
		UnitTest::Register(FName("Auxiliary"), TEST::TestAuxiliary);
	}
#endif
	
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
#if UNIT_TEST
		UnitTestInit();
#endif
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