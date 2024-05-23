#include "CommonHeader.h"
#include "Launch/Linux/LinuxLaunch.h"
#include "App/UnitTestApp.h"

int main(int argc, char* argv[])
{
	FString CommandLine = LinuxCmdLine(argc, argv);
	
	UNITTEST_UTIL::UnitTestApp app(
		{800, 600},
		*CommandLine
	);
	app.Run();
	return 0;
}
