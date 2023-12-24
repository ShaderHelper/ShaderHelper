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
		
		FString ShaderFileSource;
		FFileHelper::LoadFileToString(ShaderFileSource, *(PathHelper::ShaderDir() / "Blit.hlsl"));
		FString ShaderSource = BindGroupLayout->GetCodegenDeclaration() + ShaderFileSource;

		Vs = GpuApi::CreateShaderFromSource(ShaderType::VertexShader, ShaderSource, TEXT("BlitVS"), TEXT("MainVS"));
		Ps = GpuApi::CreateShaderFromSource(ShaderType::PixelShader, ShaderSource, TEXT("BlitPS"), TEXT("MainPS"));

		FString ErrorInfo;
		GpuApi::CrossCompileShader(Vs, ErrorInfo);
		GpuApi::CrossCompileShader(Ps, ErrorInfo);
	}

	TRefCountPtr<GpuBindGroup> BlitShader::GetBindGroup(const Parameters& InParameters) const
	{
		return GpuBindGrouprBuilder{ BindGroupLayout }
				.SetExistingBinding(0, InParameters.InputTex)
				.SetExistingBinding(1, InParameters.InputTexSampler)
				.Build();
	}

}