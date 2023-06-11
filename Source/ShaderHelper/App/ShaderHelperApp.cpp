#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Renderer/ShRenderer.h"
#include "UI/Widgets/ShaderHelperWindow/SShaderHelperWindow.h"

namespace SH {

	void ShaderHelperApp::Init()
	{
		App::Init();

		ViewPort = MakeShared<PreviewViewPort>();
		TSharedPtr<SShaderHelperWindow> ActualWindow = StaticCastSharedPtr<SShaderHelperWindow>(AppWindow);
		ActualWindow->SetViewPortInterface(ViewPort.ToSharedRef());

        if(AppRenderer.IsValid())
        {
            ShRenderer* ActualRenderer = static_cast<ShRenderer*>(AppRenderer.Get());
            ActualRenderer->ViewPort = ViewPort.Get();
            ViewPort->OnViewportResize.AddRaw(ActualRenderer, &ShRenderer::OnViewportResize);
        }
	}

	void ShaderHelperApp::PostInit()
	{
		App::PostInit();
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
