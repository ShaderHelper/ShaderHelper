#include "CommonHeaderForUE.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"


int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	SH::ShaderHelperApp app(GetCommandLineW());
	app.Run();
	return 0;
}