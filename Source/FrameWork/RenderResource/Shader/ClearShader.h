#pragma once
#include "Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
	template<BindingType InType>
	class ClearShader
	{
	public:
		ClearShader();

		GpuShader* GetComputeShader() const { return Cs; }

		TRefCountPtr<GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<GpuBindGroup> GetBindGroup(GpuResource* InResource, uint32 ResourceByteSize);

	private:
		TRefCountPtr<GpuShader> Cs;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
		UniformBufferBuilder ClearUbBuilder{ UniformBufferUsage::Temp };
	};

}
