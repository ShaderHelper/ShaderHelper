#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestConSole.h"
#include "UI/Styles/FAppCommonStyle.h"

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	UNITTEST_FRAMEWORK::UnitTestConSole app(GetCommandLineW());
	app.SetClientSize({ 800,600 });
	app.Run();
	return 0;
}
