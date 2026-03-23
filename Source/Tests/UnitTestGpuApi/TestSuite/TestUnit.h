#pragma once

#include "Editor/TestViewport.h"
#include "GpuApi/GpuRhi.h"

namespace UNITTEST_GPUAPI
{
	class TestUnit
	{
	public:
		TestUnit(FString InName)
			: Name(MoveTemp(InName))
		{
		}

		virtual ~TestUnit() = default;
		virtual void Init() {}
		virtual void Render() {};
		virtual FW::GpuTextureFormat GetViewportFormat() const
		{
			return FW::GpuTextureFormat::B8G8R8A8_UNORM;
		}

		FString Name;
		TSharedPtr<TestViewport> Viewport;
	};
}
