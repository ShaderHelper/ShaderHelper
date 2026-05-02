#include "CommonHeader.h"
#include "ScenePreviewShader.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	ScenePreviewShader::ScenePreviewShader(const std::set<FString>& VariantDefinitions)
	{
		GpuBindGroupLayoutBuilder LayoutBuilder{1};
		UbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
		UbBuilder.AddMatrix4x4f(TEXT("WorldMatrix"));
		UbBuilder.AddVector3f(TEXT("CameraPos"));
		UbBuilder.AddVector3f(TEXT("LightDir"));
		LayoutBuilder.AddUniformBuffer(TEXT("ScenePreviewUb"), UbBuilder, BindingShaderStage::Vertex | BindingShaderStage::Pixel);
		ShaderBindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = ShaderBindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ShaderHelper/ScenePreview.hlsl",
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ShaderHelper/ScenePreview.hlsl",
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> ScenePreviewShader::GetBindGroup(const FMatrix44f& ViewProjection, const FMatrix44f& WorldMatrix,
		const Vector3f& CameraPos, const Vector3f& LightDir)
	{
		GpuBindGroupBuilder Builder{ShaderBindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<FMatrix44f>(TEXT("WorldMatrix")) = WorldMatrix;
		Ub->GetMember<Vector3f>(TEXT("CameraPos")) = CameraPos;
		Ub->GetMember<Vector3f>(TEXT("LightDir")) = LightDir;
		Builder.SetUniformBuffer(TEXT("ScenePreviewUb"), Ub->GetGpuResource());
		return Builder.Build();
	}
}
