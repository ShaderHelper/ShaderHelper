#include "CommonHeader.h"
#include <Windows/WindowsHWrapper.h>
#include "App/UnitTestConSole.h"

int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	hInstance = hInInstance;
	UnitTestConSole app(GetCommandLineW());
	app.SetClientSize(FVector2D(800, 600));
	app.Run();
	return 0;
}
