#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"
#include "Renderer/ShRenderer.h"

extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 602; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\AgilitySDK\\"; }

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	SH::ShaderHelperApp app(GetCommandLineW());
	app.SetRenderer(MakeUnique<SH::ShRenderer>());
	app.Run();
	return 0;
}
