#pragma once
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class CameraWireframeShader
	{
	public:
		CameraWireframeShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetVertexShader() const { return Vs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetShaderBindGroupLayout() const { return ShaderBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const FMatrix44f& ViewProjection,
			const FMatrix44f& InvViewProjection, const FVector4f& WireColor);

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuBindGroupLayout> ShaderBindGroupLayout;
		FW::UniformBufferBuilder UbBuilder{FW::UniformBufferUsage::Temp};
	};
}
