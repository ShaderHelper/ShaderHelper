#pragma once
#include "App/App.h"
#include "Editor/UnitTestEditor.h"
#include "GpuApi/GpuRhi.h"

namespace UNITTEST_GPUAPI
{
	class UnitTestApp : public FW::App
	{
	public:
		UnitTestApp(const FW::Vector2D& InClientSize, const TCHAR* CommandLine);

	public:
		void Init() override;
		void Update(float DeltaTime) override;
		void Render() override;
	};
}

	


