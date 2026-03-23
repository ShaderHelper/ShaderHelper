#pragma once

#include "TestUnit.h"

namespace UNITTEST_GPUAPI
{
	class TestTriangleUnit : public TestUnit
	{
	public:
		TestTriangleUnit();

		void Init() override;
		void Render() override;

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
	};
}
