#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestApp.h"

ADD_AGILITY_SDK()

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	UNITTEST_GPUAPI::UnitTestApp app(
		{800, 600},
		GetCommandLineW()
	);
	app.Run();
	return 0;
}
