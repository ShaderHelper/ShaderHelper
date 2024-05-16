#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/ShaderHelperApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
		FRAMEWORK::GApp = MakeUnique<SH::ShaderHelperApp>(
			FVector2D{ 1280, 720 },
			CommandLine
		);
        FRAMEWORK::GApp->Run();
        FRAMEWORK::GApp.Reset();
	};
	[MacLaunch launch:argc argv:argv runBlock:RunBlock];
    return 0;
}
