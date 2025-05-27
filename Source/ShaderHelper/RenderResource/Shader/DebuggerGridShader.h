#pragma once
#include "GpuApi/GpuResource.h"
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class DebuggerGridShader
	{
	public:
		struct Parameters
		{
			TRefCountPtr<FW::GpuTexture> InputTex;
			
			FW::Vector2f Offset;
			uint32 Zoom;
			FW::Vector2f MouseLoc;
		};

		DebuggerGridShader();

		FW::GpuShader* GetVertexShader() const { return Vs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const Parameters& InParameters);

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;

		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		FW::UniformBufferBuilder GridUbBuilder{ FW::UniformBufferUsage::Temp };
	};

}
