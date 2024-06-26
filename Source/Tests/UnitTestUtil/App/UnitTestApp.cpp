#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "Editor/UnitTestEditor.h"

using namespace FRAMEWORK;

namespace UNITTEST_UTIL
{
	UnitTestApp::UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
	}

	void UnitTestApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

}


