#include "CommonHeader.h"
#include "BillboardShader.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	BillboardShader::BillboardShader(const std::set<FString>& VariantDefinitions)
	{
		GpuBindGroupLayoutBuilder LayoutBuilder{BindingContext::PassSlot};
		UbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
		UbBuilder.AddVector3f(TEXT("BillboardPos"));
		UbBuilder.AddFloat(TEXT("BillboardScale"));
		UbBuilder.AddVector3f(TEXT("CameraRight"));
		UbBuilder.AddVector3f(TEXT("CameraUp"));
		LayoutBuilder.AddUniformBuffer(TEXT("BillboardUb"), UbBuilder, BindingShaderStage::Vertex);
		LayoutBuilder.AddTexture(TEXT("IconTex"), BindingShaderStage::Pixel);
		LayoutBuilder.AddSampler(TEXT("IconTexSampler"), BindingShaderStage::Pixel);
		BindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Billboard.hlsl",
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});
		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Billboard.hlsl",
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> BillboardShader::GetBindGroup(const FMatrix44f& ViewProjection,
		const Vector3f& BillboardPos, float BillboardScale,
		const Vector3f& CameraRight, const Vector3f& CameraUp,
		GpuTextureView* IconTexView)
	{
		GpuBindGroupBuilder Builder{BindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<Vector3f>(TEXT("BillboardPos")) = BillboardPos;
		Ub->GetMember<float>(TEXT("BillboardScale")) = BillboardScale;
		Ub->GetMember<Vector3f>(TEXT("CameraRight")) = CameraRight;
		Ub->GetMember<Vector3f>(TEXT("CameraUp")) = CameraUp;
		Builder.SetUniformBuffer(TEXT("BillboardUb"), Ub->GetGpuResource());
		Builder.SetTexture(TEXT("IconTex"), IconTexView);
		Builder.SetSampler(TEXT("IconTexSampler"), GpuResourceHelper::GetSampler({ .Filter = SamplerFilter::Bilinear }));
		return Builder.Build();
	}
}
