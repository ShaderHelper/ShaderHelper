#include "CommonHeader.h"
#include "UnitTestEditor.h"
#include "App/UnitTestApp.h"
#include "TestSuite/TestUnit.h"
#include <Widgets/SViewport.h>

using namespace FW;

namespace UNITTEST_GPUAPI
{

	UnitTestEditor::UnitTestEditor(const FW::Vector2f& InWindowSize)
		: WindowSize(InWindowSize)
	{
	}

	void UnitTestEditor::AddTest(TUniquePtr<TestUnit> InTestUnit)
	{
		auto Viewport = MakeShared<TestViewport>(WindowSize, InTestUnit->GetViewportFormat());
		InTestUnit->Viewport = Viewport;
		InTestUnit->Init();
		Viewport->RenderFunc = [Test = InTestUnit.Get()](TestViewport*) {
			Test->Render();
		};
		auto TestWindow = SNew(SWindow)
			.Title(FText::FromString(InTestUnit->Name))
			.ClientSize(WindowSize)
			[
				SNew(SViewport)
				.ViewportInterface(Viewport)
			];
		FSlateApplication::Get().AddWindow(TestWindow);
		static_cast<UnitTestApp*>(GApp.Get())->TestUnits.Add(MoveTemp(InTestUnit));
	}


}
