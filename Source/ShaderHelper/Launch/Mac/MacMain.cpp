#include "CommonHeader.h"
#include "Launch/Mac/MacLaunch.h"
#include "App/ShaderHelperApp.h"

int main(int argc, char *argv[])
{
	auto RunBlock = [](const TCHAR* CommandLine)
	{
		SH::ShaderHelperApp app(
			{ 1280, 720 },
			CommandLine
		);
		app.Run();
	};
	[MacLaunch launch:argc argv:argv runBlock:RunBlock];
    return 0;
}
