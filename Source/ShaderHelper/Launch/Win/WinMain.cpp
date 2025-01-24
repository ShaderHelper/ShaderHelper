#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"
#include "Launch/Win/WinLaunch.h"

//Add these info for AgilitySDK.
ADD_AGILITY_SDK()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	WinLaunch([](const TCHAR* CommandLine)
		{
			FW::GApp = MakeUnique<SH::ShaderHelperApp>(
				FVector2D{ 1600, 800 },
				CommandLine
			);
			FW::GApp->Run();
		}
	, GetCommandLineW());

	return 0;
}
