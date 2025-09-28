#pragma once
#include "GpuApi/GpuResource.h"
#include "Shader.h"

namespace FW
{
	class BlitShader
	{
	public:
		struct Parameters
		{
			TRefCountPtr<GpuTexture> InputTex;
			TRefCountPtr<GpuSampler> InputTexSampler;
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
	};

}
