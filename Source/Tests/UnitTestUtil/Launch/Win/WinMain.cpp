#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestApp.h"
#include "Launch/Win/WinLaunch.h"

ADD_AGILITY_SDK()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	WinLaunch([](const TCHAR* CommandLine)
		{
			FRAMEWORK::GApp = MakeUnique<UNITTEST_UTIL::UnitTestApp>(
				FVector2D{ 800, 600 },
				CommandLine
			);
			FRAMEWORK::GApp->Run();
		}
	, GetCommandLineW());
	return 0;
}
