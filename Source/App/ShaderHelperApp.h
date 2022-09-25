#pragma once
#include "CommonHeader.h"
#include "FrameWork/App.h"
namespace SH {
	
	class ShaderHelperApp : public App
	{
	public:
		ShaderHelperApp(const TCHAR* CommandLine);
	public:

		void Init();
		void ShutDown();
		void PostInit();
		void Update(double DeltaTime);
	};
	
}


