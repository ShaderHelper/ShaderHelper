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

		DebuggerGridShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetVertexShader() const { return Vs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetShaderBindGroupLayout() const { return ShaderBindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const Parameters& InParameters);

	private:
		TRefCountPtr<FW::GpuShader> Vs;
		TRefCountPtr<FW::GpuShader> Ps;

		TRefCountPtr<FW::GpuBindGroupLayout> ShaderBindGroupLayout;
		FW::UniformBufferBuilder GridUbBuilder{ FW::UniformBufferUsage::Temp };
	};

}
