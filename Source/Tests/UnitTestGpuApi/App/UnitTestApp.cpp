#include "CommonHeader.h"
#include "UnitTestApp.h"
#include "TestSuite/TestCastUnit.h"
#include "TestSuite/TestCubeUnit.h"
#include "TestSuite/TestTriangleUnit.h"
#include "Editor/UnitTestEditor.h"


using namespace FW;

namespace UNITTEST_GPUAPI
{
	UnitTestApp::UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		
	}

	void UnitTestApp::Init()
	{
		App::Init();
		
		AppEditor = MakeUnique<UnitTestEditor>(AppClientSize);
		auto* Editor = static_cast<UnitTestEditor*>(AppEditor.Get());
		//Add test otherwise the run loop will immediately exit.
		//Editor->AddTest(MakeUnique<TestCastUnit>());
		//Editor->AddTest(MakeUnique<TestTriangleUnit>());
		Editor->AddTest(MakeUnique<TestCubeUnit>());
	}

	void UnitTestApp::Update(float DeltaTime)
	{
		App::Update(DeltaTime);
	}

	void UnitTestApp::Render()
	{
		App::Render();

		for (const auto& TestUnit: TestUnits)
		{
			TestUnit->Render();
		}
	}

}


