#pragma once
#include <set>
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class OutlineShader
	{
	public:
		OutlineShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetMaskVS() const { return MaskVs; }
		FW::GpuShader* GetMaskPS() const { return MaskPs; }
		TRefCountPtr<FW::GpuBindGroupLayout> GetMaskShaderBindGroupLayout() const { return MaskShaderBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetMaskBindGroup(const FMatrix44f& ViewProjection, const FMatrix44f& WorldMatrix);

		FW::GpuShader* GetPostVS() const { return PostVs; }
		FW::GpuShader* GetPostPS() const { return PostPs; }
		TRefCountPtr<FW::GpuBindGroupLayout> GetPostShaderBindGroupLayout() const { return PostShaderBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetPostBindGroup(FW::GpuTextureView* MaskTexView, const FW::Vector4f& OutlineColor, const FW::Vector2f& TexelSize);

	private:
		TRefCountPtr<FW::GpuShader> MaskVs;
		TRefCountPtr<FW::GpuShader> MaskPs;
		TRefCountPtr<FW::GpuBindGroupLayout> MaskShaderBindGroupLayout;
		FW::UniformBufferBuilder MaskUbBuilder{FW::UniformBufferUsage::Temp};

		TRefCountPtr<FW::GpuShader> PostVs;
		TRefCountPtr<FW::GpuShader> PostPs;
		TRefCountPtr<FW::GpuBindGroupLayout> PostShaderBindGroupLayout;
		FW::UniformBufferBuilder PostUbBuilder{FW::UniformBufferUsage::Temp};
	};
}
