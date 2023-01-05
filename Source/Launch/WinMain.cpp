#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"

//Ensures the consistency of memory allocation between current project and UE modules.
PER_MODULE_DEFINITION()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	SH::ShaderHelperApp app(GetCommandLineW());
	app.Run();
	return 0;
}
