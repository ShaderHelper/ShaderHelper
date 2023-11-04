#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/UnitTestConSole.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
		UNITTEST_FRAMEWORK::UnitTestConSole app(CommandLine);
		app.SetClientSize({ 800,600 });
		app.Run();
	};
	[MacLaunch launch : argc argv : argv runBlock : RunBlock] ;
	return 0;
}
