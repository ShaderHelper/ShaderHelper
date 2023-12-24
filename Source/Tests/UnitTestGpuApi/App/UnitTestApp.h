#pragma once
#include "App/App.h"
#include "Editor/UnitTestEditor.h"

namespace UNITTEST_GPUAPI
{
	class UnitTestApp : public App
	{
	public:
		UnitTestApp(const Vector2D& InClientSize, const TCHAR* CommandLine);
		void Update(double DeltaTime) override;
	};
}

	


