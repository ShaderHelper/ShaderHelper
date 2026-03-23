#pragma once

#include "TestUnit.h"

namespace UNITTEST_GPUAPI
{
	class TestCastUnit : public TestUnit
	{
	public:
		TestCastUnit();

		void Init() override;
		void Render() override;

	private:
		TRefCountPtr<FW::GpuTexture> TestTex;
		TRefCountPtr<FW::GpuTexture> DummyRT;
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<FW::GpuBindGroup> BindGroup;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
	};
}
