#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/UnitTestApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
        FRAMEWORK::GApp = MakeUnique<UNITTEST_UTIL::UnitTestApp>(
            FVector2D{ 800, 600 },
            CommandLine
        );
        FRAMEWORK::GApp->Run();
        FRAMEWORK::GApp.Reset();
	};
	[MacLaunch launch : argc argv : argv runBlock : RunBlock] ;
	return 0;
}
