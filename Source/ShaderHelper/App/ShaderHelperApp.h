#pragma once
#include "FrameWork/App/App.h"
namespace SH {
	
	class ShaderHelperApp : public App
	{
	public:
		ShaderHelperApp(const TCHAR* CommandLine);
	private:
		void Init() override;
		void ShutDown() override;
		void PostInit() override;
		void Update(double DeltaTime) override;
	};
	
}


