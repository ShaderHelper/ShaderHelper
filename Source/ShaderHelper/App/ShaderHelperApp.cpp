#include "CommonHeader.h"
#include "ShaderHelperApp.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Renderer/ShRenderer.h"
#include "UI/Widgets/ShaderHelperWindow/SShaderHelperWindow.h"
#include "ProjectManager/ShProjectManager.h"
#include "Common/Path/PathHelper.h"

namespace SH {

	const FString DefaultProjectPath = PathHelper::InitialDir() / TEXT("TemplateProject/Default/Default.shprj");

	void ShaderHelperApp::Init()
	{
		App::Init();
		FShaderHelperStyle::Init();
		//Need to schedule when to call RequestEngineExit to exit the app
		FSlateApplication::Get().SetExitRequestedHandler(nullptr);

		TSingleton<ShProjectManager>::Get().OpenProject(DefaultProjectPath);

		ViewPort = MakeShared<PreviewViewPort>();
		AppRenderer = MakeUnique<ShRenderer>(ViewPort.Get());
		ViewPort->OnViewportResize.AddRaw(static_cast<ShRenderer*>(AppRenderer.Get()), &ShRenderer::OnViewportResize);
		
		InitEditorUI();
	}

	void ShaderHelperApp::InitEditorUI()
	{
		TSharedRef<SShaderHelperWindow> EditorWindow = SNew(SShaderHelperWindow)
			.Renderer(static_cast<ShRenderer*>(AppRenderer.Get()))
			.OnResetWindowLayout_Lambda([this] { IsReInitEditorUI = true; })
			.WindowSize(AppClientSize);

		EditorWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&) {
			if (!IsReInitEditorUI)
			{
				RequestEngineExit(TEXT("Normal Slate Window Closed"));
			}
			else
			{
				ReInitEditorUi();
			}
		}));

		FSlateApplication::Get().AddWindow(EditorWindow);
		EditorWindow->SetViewPortInterface(ViewPort.ToSharedRef());
	}

	void ShaderHelperApp::ReInitEditorUi()
	{
		InitEditorUI();
		IsReInitEditorUI = false;
	}

	void ShaderHelperApp::InitLauncherUI()
	{

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
