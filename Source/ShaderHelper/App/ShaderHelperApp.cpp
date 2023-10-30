#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Renderer/ShRenderer.h"
#include "UI/Widgets/ShaderHelperWindow/SShaderHelperWindow.h"

namespace SH {

	void ShaderHelperApp::Init()
	{
		App::Init();
		FShaderHelperStyle::Init();

		ViewPort = MakeShared<PreviewViewPort>();
		AppRenderer = MakeUnique<ShRenderer>(ViewPort.Get());

		AppWindow = SNew(SShaderHelperWindow)
			.Renderer(static_cast<ShRenderer*>(AppRenderer.Get()))
			.WindowSize(AppClientSize);

		ViewPort->OnViewportResize.AddRaw(static_cast<ShRenderer*>(AppRenderer.Get()), &ShRenderer::OnViewportResize);
		StaticCastSharedPtr<SShaderHelperWindow>(AppWindow)->SetViewPortInterface(ViewPort.ToSharedRef());
	}

	void ShaderHelperApp::PostInit()
	{
		App::PostInit();
		FSlateApplication::Get().AddWindow(AppWindow.ToSharedRef());
	}


	void ShaderHelperApp::ShutDown()
	{
		App::ShutDown();
		FShaderHelperStyle::ShutDown();
	}


	void ShaderHelperApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}

}
