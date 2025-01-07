#pragma once
#include "App/App.h"
#include "Editor/UnitTestEditor.h"

namespace UNITTEST_UTIL
{
	class UnitTestApp : public FW::App
	{
	public:
		UnitTestApp(const FW::Vector2D& InClientSize, const TCHAR* CommandLine);

	public:
		void Init() override;
		void Update(double DeltaTime) override;
	};
}

	


