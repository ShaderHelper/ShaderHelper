#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "Renderer/ShRenderer.h"

namespace SH {

	ShaderHelperApp::ShaderHelperApp(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: App(InClientSize, CommandLine)
	{
		//Need to schedule when to call RequestEngineExit to exit the app
		FSlateApplication::Get().SetExitRequestedHandler(nullptr);

		Renderer = MakeUnique<ShRenderer>();
		InitEditor();
	}

	ShaderHelperApp::~ShaderHelperApp()
	{

	}

	void ShaderHelperApp::InitEditor()
	{
		Editor.Reset();
		Editor = MakeUnique<ShaderHelperEditor>(AppClientSize, Renderer.Get());
		Editor->OnResetWindowLayout.BindLambda([this] { IsReInitEditor = true; });
		Editor->OnWindowClosed.BindLambda([this] {
			if (!IsReInitEditor)
			{
				RequestEngineExit(TEXT("Normal Slate Window Closed"));
			}
			else
			{
				ReInitEditor();
			}
		});
	}

	void ShaderHelperApp::ReInitEditor()
	{
		InitEditor();
		IsReInitEditor = false;
	}

	void ShaderHelperApp::Update(double DeltaTime)
	{
		App::Update(DeltaTime);

		Renderer->Render();
		Editor->Update(DeltaTime);
	}

}
