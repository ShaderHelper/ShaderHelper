#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"

//Add these info for AgilitySDK.
ADD_AGILITY_SDK()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	SH::ShaderHelperApp app(
		{ 1600,800 },
		GetCommandLineW()
	);
	app.Run();
	return 0;
}
