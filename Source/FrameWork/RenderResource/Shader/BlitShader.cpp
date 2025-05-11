#include "CommonHeader.h"
#include "BlitShader.h"
#include "GpuApi/GpuRhi.h"
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"

namespace FW
{

	BlitShader::BlitShader()
	{
		BindGroupLayout = GpuBindGroupLayoutBuilder{ BindingContext::ShaderSlot }
							.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Pixel)
							.AddExistingBinding(1, BindingType::Sampler, BindingShaderStage::Pixel)
							.Build();
		
		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Blit.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS"
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Blit.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS"
		});

		FString ErrorInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		GGpuRhi->CompileShader(Ps, ErrorInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> BlitShader::GetBindGroup(const Parameters& InParameters) const
	{
		return GpuBindGroupBuilder{ BindGroupLayout }
				.SetExistingBinding(0, InParameters.InputTex)
				.SetExistingBinding(1, InParameters.InputTexSampler)
				.Build();
	}

}
