#include "CommonHeader.h"
#include "BlitShader.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

namespace FW
{

	BlitShader::BlitShader(const std::set<FString>& VariantDefinitions)
	{
		TArray<FString> ExtraArgs;
		for(const auto& D : VariantDefinitions)
		{
			ExtraArgs.Add("-D");
			ExtraArgs.Add(D);
		}

		bUseMipLevel = VariantDefinitions.count(TEXT("USE_MIP_LEVEL")) > 0;

		GpuBindGroupLayoutBuilder LayoutBuilder{ BindingContext::ShaderSlot };
		LayoutBuilder.AddExistingBinding(0, BindingType::Texture, BindingShaderStage::Pixel)
			.AddExistingBinding(1, BindingType::Sampler, BindingShaderStage::Pixel);

		if (bUseMipLevel)
		{
			BlitUbBuilder.AddFloat(TEXT("BlitMipLevel"));
			LayoutBuilder.AddUniformBuffer(TEXT("BlitUb"), BlitUbBuilder, BindingShaderStage::Pixel);
		}

		BindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Blit.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Blit.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo, ExtraArgs);
		check(ErrorInfo.IsEmpty());

		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo, ExtraArgs);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> BlitShader::GetBindGroup(const Parameters& InParameters)
	{
		GpuBindGroupBuilder Builder{ BindGroupLayout };
		Builder.SetExistingBinding(0, BindingType::Texture, InParameters.InputView)
			.SetExistingBinding(1, BindingType::Sampler, InParameters.InputTexSampler);

		if (bUseMipLevel)
		{
			TUniquePtr<UniformBuffer> BlitUb = BlitUbBuilder.Build();
			BlitUb->GetMember<float>(TEXT("BlitMipLevel")) = InParameters.MipLevel;
			Builder.SetUniformBuffer(TEXT("BlitUb"), BlitUb->GetGpuResource());
		}

		return Builder.Build();
	}

}
