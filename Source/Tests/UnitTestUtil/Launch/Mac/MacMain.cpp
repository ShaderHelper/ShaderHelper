#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/UnitTestApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
		UNITTEST_UTIL::UnitTestApp app(
			{ 800,600 },
			CommandLine
		);
		app.Run();
	};
	[MacLaunch launch : argc argv : argv runBlock : RunBlock] ;
	return 0;
}
