#pragma once
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

namespace SH {
	
	class ShaderHelperApp : public App
	{
	public:
		ShaderHelperApp(const Vector2D& InClientSize, const TCHAR* CommandLine);
		~ShaderHelperApp();

	private:
		void InitEditor();
		void Update(double DeltaTime) override;
		
	private:
		TUniquePtr<ShRenderer> Renderer;
		TUniquePtr<ShaderHelperEditor> Editor;
	};
	
}


