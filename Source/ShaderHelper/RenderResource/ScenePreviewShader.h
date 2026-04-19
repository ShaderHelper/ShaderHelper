#pragma once
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class ScenePreviewShader
	{
	public:
		ScenePreviewShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetVertexShader() const { return Vs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const FMatrix44f& ViewProjection, const FMatrix44f& WorldMatrix, 
			const FW::Vector3f& CameraPos, const FW::Vector3f& LightDir);

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		FW::UniformBufferBuilder UbBuilder{FW::UniformBufferUsage::Temp};
	};
}
