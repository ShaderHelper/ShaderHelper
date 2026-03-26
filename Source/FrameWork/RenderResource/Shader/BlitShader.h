#pragma once
#include "GpuApi/GpuResource.h"
#include "RenderResource/UniformBuffer.h"
#include "Shader.h"

namespace FW
{
	class BlitShader
	{
	public:
		struct Parameters
		{
			TRefCountPtr<GpuTextureView> InputView;
			TRefCountPtr<GpuSampler> InputTexSampler;
			float MipLevel = 0.0f;
		};

		BlitShader(const std::set<FString>& VariantDefinitions = {});

		GpuShader* GetVertexShader() const { return Vs; }
		GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<GpuBindGroup> GetBindGroup(const Parameters& InParameters);

	private:
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;

		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
		UniformBufferBuilder BlitUbBuilder{UniformBufferUsage::Temp};
		bool bUseMipLevel = false;
	};

}
