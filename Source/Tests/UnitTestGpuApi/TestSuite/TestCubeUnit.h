#pragma once

#include "TestUnit.h"
#include "RenderResource/Camera.h"
#include "RenderResource/Mesh.h"
#include "RenderResource/UniformBuffer.h"

namespace UNITTEST_GPUAPI
{
	class TestCubeUnit : public TestUnit
	{
	public:
		TestCubeUnit();

		void Init() override;
		void Render() override;

	private:
		bool bDragging = false;
		FW::Vector2f LastMousePos = { 0.0f, 0.0f };
		FW::Camera ViewCamera;
		float ModelYaw = 0.0f;
		float ModelPitch = 0.0f;
		FW::MeshBuffers PrimitiveBuffers;
		TUniquePtr<FW::UniformBuffer> UniformBuffer;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<FW::GpuBindGroup> BindGroup;
		TRefCountPtr<FW::GpuRenderPipelineState> Pipeline;
	};
}
