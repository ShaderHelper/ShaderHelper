#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/ShaderHelperApp.h"
#include "Renderer/ShRenderer.h"
#include "UI/ShaderHelperWindow/Widgets/SShaderHelperWindow.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Styles/FAppCommonStyle.h"

extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 602; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\AgilitySDK\\"; }

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	SH::UE_Init(GetCommandLineW());
	SH::FAppCommonStyle::Init();
	SH::FShaderHelperStyle::Init();
	
	SH::ShaderHelperApp::SetClientSize({1024,576});
	SH::ShaderHelperApp app(SNew(SH::SShaderHelperWindow), MakeUnique<SH::ShRenderer>());
	app.Run();
	
	SH::UE_ShutDown();
	return 0;
}
