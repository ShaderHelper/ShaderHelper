#pragma once
#include "App/App.h"
#include "Editor/UnitTestEditor.h"
#include "GpuApi/GpuRhi.h"

namespace UNITTEST_GPUAPI
{
	class UnitTestApp : public FRAMEWORK::App
	{
	public:
		UnitTestApp(const FRAMEWORK::Vector2D& InClientSize, const TCHAR* CommandLine);
		void Update(double DeltaTime) override;
		void Render() override;

	private:
		TUniquePtr<UnitTestEditor> Editor;
	};
}

	


