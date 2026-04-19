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

		// Pass 1: Mask
		FW::GpuShader* GetMaskVS() const { return MaskVs; }
		FW::GpuShader* GetMaskPS() const { return MaskPs; }
		TRefCountPtr<FW::GpuBindGroupLayout> GetMaskBindGroupLayout() const { return MaskBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetMaskBindGroup(const FMatrix44f& ViewProjection, const FMatrix44f& WorldMatrix);

		// Pass 2: Post-process edge detection
		FW::GpuShader* GetPostVS() const { return PostVs; }
		FW::GpuShader* GetPostPS() const { return PostPs; }
		TRefCountPtr<FW::GpuBindGroupLayout> GetPostBindGroupLayout() const { return PostBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetPostBindGroup(FW::GpuTextureView* MaskTexView, const FW::Vector4f& OutlineColor, const FW::Vector2f& TexelSize);

	private:
		// Mask pass
		TRefCountPtr<FW::GpuShader> MaskVs;
		TRefCountPtr<FW::GpuShader> MaskPs;
		TRefCountPtr<FW::GpuBindGroupLayout> MaskBindGroupLayout;
		FW::UniformBufferBuilder MaskUbBuilder{FW::UniformBufferUsage::Temp};

		// Post pass
		TRefCountPtr<FW::GpuShader> PostVs;
		TRefCountPtr<FW::GpuShader> PostPs;
		TRefCountPtr<FW::GpuBindGroupLayout> PostBindGroupLayout;
		FW::UniformBufferBuilder PostUbBuilder{FW::UniformBufferUsage::Temp};
	};
}
