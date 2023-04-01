#pragma once
#include "App/App.h"

namespace UNITTEST_FRAMEWORK
{
	class UnitTestConSole : public App
	{
	public:
		using App::App;
		
	private:
		void Init() override;
		void ShutDown() override;
		void PostInit() override;
		void Update(double DeltaTime) override;

		void InitLogWindow();
	};
}

	


