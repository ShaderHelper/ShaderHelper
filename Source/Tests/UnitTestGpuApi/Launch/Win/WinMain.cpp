#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestApp.h"

ADD_AGILITY_SDK()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
    FRAMEWORK::GApp = MakeUnique<UNITTEST_GPUAPI::UnitTestApp>(
       FVector2D{ 800, 600 },
       GetCommandLineW()
    );
    FRAMEWORK::GApp.Run();
	return 0;
}
