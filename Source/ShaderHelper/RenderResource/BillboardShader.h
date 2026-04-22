#pragma once
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class BillboardShader
	{
	public:
		BillboardShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetVertexShader() const { return Vs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const FMatrix44f& ViewProjection,
			const FW::Vector3f& BillboardPos, float BillboardScale,
			const FW::Vector3f& CameraRight, const FW::Vector3f& CameraUp,
			FW::GpuTextureView* IconTexView);

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		FW::UniformBufferBuilder UbBuilder{FW::UniformBufferUsage::Temp};
	};
}
