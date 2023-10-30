#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"

//Add these info for AgilitySDK.
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 602; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\AgilitySDK\\"; }

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	SH::ShaderHelperApp app(GetCommandLineW());
	app.SetClientSize({ 1366,768 });
	app.Run();
	return 0;
}
