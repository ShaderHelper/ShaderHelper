#include "CommonHeaderForUE.h"
#include "SShaderHelperWindow.h"
#include "App/ShaderHelperApp.h"

#define LOCTEXT_NAMESPACE "ShaderHelperWindow"

namespace SH {

	SShaderHelperWindow::SShaderHelperWindow()
	{

	}

	SShaderHelperWindow::~SShaderHelperWindow()
	{

	}

	void SShaderHelperWindow::Construct(const FArguments& InArgs)
	{
		SWindow::Construct(SWindow::FArguments()
			.Title(LOCTEXT("ShaderHelperTitle","ShaderHelper"))
			.ClientSize(ShaderHelperApp::DefaultClientSize)
			);
			
	}

	TSharedRef<SWidget> SShaderHelperWindow::CreateMenuBar()
	{

	}

	void SShaderHelperWindow::FillFileMenu()
	{

	}

	void SShaderHelperWindow::FillConfigMenu()
	{

	}

	void SShaderHelperWindow::FillWindowMenu()
	{

	}

}