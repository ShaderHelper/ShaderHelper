#pragma once
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class SceneGridShader
	{
	public:
		SceneGridShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetGridVS() const { return GridVs; }
		FW::GpuShader* GetPS() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const FMatrix44f& ViewProjection, const FW::Vector3f& CameraPos, float GridSize, float GridSpacing);

	private:
		TRefCountPtr<FW::GpuShader> GridVs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		FW::UniformBufferBuilder UbBuilder{FW::UniformBufferUsage::Temp};
	};
}
