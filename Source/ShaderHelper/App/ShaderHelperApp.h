#pragma once
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

namespace SH {
	
	class ShaderHelperApp : public FRAMEWORK::App
	{
	public:
		ShaderHelperApp(const FRAMEWORK::Vector2D& InClientSize, const TCHAR* CommandLine);
		~ShaderHelperApp();

	private:
		void InitEditor();
		void Update(double DeltaTime) override;
		void Render() override;

	private:
		TUniquePtr<ShRenderer> Renderer;
		TUniquePtr<ShaderHelperEditor> Editor;
	};
	
}


