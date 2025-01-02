#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/ShaderHelperApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
		FW::GApp = MakeUnique<SH::ShaderHelperApp>(
			FVector2D{ 1280, 720 },
			CommandLine
		);
        FW::GApp->Run();
        FW::GApp.Reset();
	};
	[MacLaunch launch:argc argv:argv runBlock:RunBlock];
    return 0;
}
