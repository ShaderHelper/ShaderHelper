#include "CommonHeader.h"
#include "UnitTestConSole.h"


UnitTestConSole::UnitTestConSole(const TCHAR* CommandLine)
	:App(CommandLine)
{
	bEnableConsoleOutput = true;
}

void UnitTestConSole::Init()
{
	App::Init();
}

void UnitTestConSole::PostInit()
{
	App::PostInit();
	extern void TestUtil();
	TestUtil();
}


void UnitTestConSole::ShutDown()
{
	App::ShutDown();
}


void UnitTestConSole::Update(double DeltaTime)
{
	App::Update(DeltaTime);
}
