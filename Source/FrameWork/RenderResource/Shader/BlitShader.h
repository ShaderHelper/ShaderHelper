#pragma once
#include "GpuApi/GpuResource.h"
#include "Shader.h"

namespace FRAMEWORK
{
	class BlitShader : public Shader
	{
	public:
		struct Parameters
		{
			GpuTexture* InputTex;
			GpuSampler* InputTexSampler;
		};

		BlitShader();

		GpuShader* GetVertexShader() const { return Vs; }
		GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<GpuBindGroup> GetBindGroup(const Parameters& InParameters) const;

	private:
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;

		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
	};

}