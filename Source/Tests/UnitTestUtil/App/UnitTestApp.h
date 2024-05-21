#pragma once
#include "App/App.h"
#include "Editor/UnitTestEditor.h"

namespace UNITTEST_UTIL
{
	class UnitTestApp : public FRAMEWORK::App
	{
	public:
		UnitTestApp(const FRAMEWORK::Vector2D& InClientSize, const TCHAR* CommandLine);
		void Update(double DeltaTime) override;
	};
}

	


