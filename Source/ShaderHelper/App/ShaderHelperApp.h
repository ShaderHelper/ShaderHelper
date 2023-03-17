#pragma once
#include "App/App.h"
namespace SH {
	
	class ShaderHelperApp : public App
	{
	public:
		using App::App;
	private:
		void Init() override;
		void ShutDown() override;
		void PostInit() override;
		void Update(double DeltaTime) override;
	};
	
}


