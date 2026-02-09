#include "CommonHeader.h"
#include "UnitTestEditor.h"
#include "App/UnitTestApp.h"
#include <Widgets/SViewport.h>

using namespace FW;

namespace UNITTEST_GPUAPI
{

	UnitTestEditor::UnitTestEditor(const FW::Vector2f& InWindowSize)
		: WindowSize(InWindowSize)
	{
	}

	void UnitTestEditor::AddTest(const FString& InName, const TFunction<void(TestViewport*)> InTest, FW::GpuTextureFormat ViewportFormat)
	{
		auto Viewport = MakeShared<TestViewport>(WindowSize, ViewportFormat);
		Viewport->RenderFunc = InTest;
		auto TestWindow = SNew(SWindow)
			.Title(FText::FromString(InName))
			.ClientSize(WindowSize)
			[
				SNew(SViewport)
				.ViewportInterface(Viewport)
			];
		FSlateApplication::Get().AddWindow(TestWindow);
		static_cast<UnitTestApp*>(GApp.Get())->TestUnits.Add({
			.Name = InName,
			.TestFunc = InTest,
			.Viewport = Viewport
		});
	}


}
