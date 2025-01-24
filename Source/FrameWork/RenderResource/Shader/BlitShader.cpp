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
		
		Vs = GGpuRhi->CreateShaderFromFile(
			PathHelper::ShaderDir() / "Blit.hlsl", 
			ShaderType::VertexShader,
			TEXT("MainVS")
		);

		Ps = GGpuRhi->CreateShaderFromFile(
			PathHelper::ShaderDir() / "Blit.hlsl",
			ShaderType::PixelShader,
			TEXT("MainPS")
		);

		FString ErrorInfo;
		GGpuRhi->CrossCompileShader(Vs, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		GGpuRhi->CrossCompileShader(Ps, ErrorInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> BlitShader::GetBindGroup(const Parameters& InParameters) const
	{
		return GpuBindGrouprBuilder{ BindGroupLayout }
				.SetExistingBinding(0, InParameters.InputTex)
				.SetExistingBinding(1, InParameters.InputTexSampler)
				.Build();
	}

}
