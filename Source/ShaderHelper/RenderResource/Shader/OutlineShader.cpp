#include "CommonHeader.h"
#include "OutlineShader.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	OutlineShader::OutlineShader(const std::set<FString>& VariantDefinitions)
	{
		{
			GpuBindGroupLayoutBuilder LayoutBuilder{1};
			MaskUbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
			MaskUbBuilder.AddMatrix4x4f(TEXT("WorldMatrix"));
			LayoutBuilder.AddUniformBuffer(TEXT("OutlineUb"), MaskUbBuilder, BindingShaderStage::Vertex);
			MaskShaderBindGroupLayout = LayoutBuilder.Build();

			FString ExtraDecl = MaskShaderBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

			MaskVs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "ShaderHelper/OutlineMask.hlsl",
				.Type = ShaderType::Vertex,
				.EntryPoint = "MaskVS",
				.ExtraDecl = ExtraDecl,
			});

			MaskPs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "ShaderHelper/OutlineMask.hlsl",
				.Type = ShaderType::Pixel,
				.EntryPoint = "MaskPS",
				.ExtraDecl = ExtraDecl,
			});

			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(MaskVs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
			GGpuRhi->CompileShader(MaskPs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
		}

		{
			GpuBindGroupLayoutBuilder LayoutBuilder{1};
			PostUbBuilder.AddVector4f(TEXT("OutlineColor"));
			PostUbBuilder.AddVector2f(TEXT("TexelSize"));
			LayoutBuilder.AddUniformBuffer(TEXT("OutlineUb"), PostUbBuilder, BindingShaderStage::Pixel);
			LayoutBuilder.AddTexture(TEXT("MaskTex"), BindingShaderStage::Pixel);
			LayoutBuilder.AddSampler(TEXT("MaskTexSampler"), BindingShaderStage::Pixel);
			PostShaderBindGroupLayout = LayoutBuilder.Build();

			FString ExtraDecl = PostShaderBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

			PostVs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "ShaderHelper/OutlinePost.hlsl",
				.Type = ShaderType::Vertex,
				.EntryPoint = "PostVS",
				.ExtraDecl = ExtraDecl,
			});

			PostPs = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "ShaderHelper/OutlinePost.hlsl",
				.Type = ShaderType::Pixel,
				.EntryPoint = "PostPS",
				.ExtraDecl = ExtraDecl,
			});

			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(PostVs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
			GGpuRhi->CompileShader(PostPs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
		}
	}

	TRefCountPtr<GpuBindGroup> OutlineShader::GetMaskBindGroup(const FMatrix44f& ViewProjection, const FMatrix44f& WorldMatrix)
	{
		GpuBindGroupBuilder Builder{MaskShaderBindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = MaskUbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<FMatrix44f>(TEXT("WorldMatrix")) = WorldMatrix;
		Builder.SetUniformBuffer(TEXT("OutlineUb"), Ub->GetGpuResource());
		return Builder.Build();
	}

	TRefCountPtr<GpuBindGroup> OutlineShader::GetPostBindGroup(GpuTextureView* MaskTexView, const Vector4f& OutlineColor, const Vector2f& TexelSize)
	{
		GpuBindGroupBuilder Builder{PostShaderBindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = PostUbBuilder.Build();
		Ub->GetMember<Vector4f>(TEXT("OutlineColor")) = OutlineColor;
		Ub->GetMember<Vector2f>(TEXT("TexelSize")) = TexelSize;
		Builder.SetUniformBuffer(TEXT("OutlineUb"), Ub->GetGpuResource());
		Builder.SetTexture(TEXT("MaskTex"), MaskTexView);
		Builder.SetSampler(TEXT("MaskTexSampler"), GpuResourceHelper::GetSampler({ .Filter = SamplerFilter::Bilinear }));
		return Builder.Build();
	}
}
