#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "Editor/UnitTestEditor.h"

using namespace FW;

namespace UNITTEST_UTIL
{
	UnitTestApp::UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		
	}

	void UnitTestApp::Init()
	{
		App::Init();
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
	}

	void UnitTestApp::Update(float DeltaTime)
	{
		App::Update(DeltaTime);
	}

}


