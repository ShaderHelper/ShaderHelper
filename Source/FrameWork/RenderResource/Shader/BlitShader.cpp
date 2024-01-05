#include "CommonHeader.h"
#include "BlitShader.h"
#include "GpuApi/GpuApiInterface.h"
#include <Core/Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"

namespace FRAMEWORK
{

	BlitShader::BlitShader()
	{
		BindGroupLayout = GpuBindGroupLayoutBuilder{ BindingContext::ShaderSlot }
							.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Pixel)
							.AddExistingBinding(1, BindingType::Sampler, BindingShaderStage::Pixel)
							.Build();
		
		Vs = GpuApi::CreateShaderFromFile(
			PathHelper::ShaderDir() / "Blit.hlsl", 
			ShaderType::VertexShader,
			TEXT("MainVS")
		);

		Ps = GpuApi::CreateShaderFromFile(
			PathHelper::ShaderDir() / "Blit.hlsl",
			ShaderType::PixelShader,
			TEXT("MainPS")
		);

		FString ErrorInfo;
		GpuApi::CrossCompileShader(Vs, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		GpuApi::CrossCompileShader(Ps, ErrorInfo);
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
