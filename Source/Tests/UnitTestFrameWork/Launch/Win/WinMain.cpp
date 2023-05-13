#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestConSole.h"
#include "UI/Styles/FAppCommonStyle.h"

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	UNITTEST_FRAMEWORK::UE_Init(GetCommandLineW());
	UNITTEST_FRAMEWORK::FAppCommonStyle::Init();

	UNITTEST_FRAMEWORK::UnitTestConSole::SetClientSize({ 800, 600 });
	UNITTEST_FRAMEWORK::UnitTestConSole app;
	app.Run();
	
	UNITTEST_FRAMEWORK::UE_ShutDown();
	return 0;
}
