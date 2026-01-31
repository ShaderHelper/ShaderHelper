#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/UnitTestApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
        FW::GApp = MakeUnique<UNITTEST_GPUAPI::UnitTestApp>(
            FVector2D{ 400, 300 },
            CommandLine
        );
        FW::GApp->Run();
	};
	[MacLaunch launch : argc argv : argv runBlock : RunBlock] ;
	return 0;
}
