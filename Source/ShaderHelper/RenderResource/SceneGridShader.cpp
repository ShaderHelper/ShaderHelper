#include "CommonHeader.h"
#include "SceneGridShader.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	SceneGridShader::SceneGridShader(const std::set<FString>& VariantDefinitions)
	{
		GpuBindGroupLayoutBuilder LayoutBuilder{BindingContext::PassSlot};
		UbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
		UbBuilder.AddVector3f(TEXT("CameraPos"));
		UbBuilder.AddFloat(TEXT("GridSize"));
		UbBuilder.AddFloat(TEXT("GridSpacing"));
		LayoutBuilder.AddUniformBuffer(TEXT("SceneGridUb"), UbBuilder, BindingShaderStage::Vertex | BindingShaderStage::Pixel);
		BindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		GridVs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "SceneGrid.hlsl",
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "SceneGrid.hlsl",
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(GridVs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> SceneGridShader::GetBindGroup(const FMatrix44f& ViewProjection, const Vector3f& CameraPos, float GridSize, float GridSpacing)
	{
		GpuBindGroupBuilder Builder{BindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<Vector3f>(TEXT("CameraPos")) = CameraPos;
		Ub->GetMember<float>(TEXT("GridSize")) = GridSize;
		Ub->GetMember<float>(TEXT("GridSpacing")) = GridSpacing;
		Builder.SetUniformBuffer(TEXT("SceneGridUb"), Ub->GetGpuResource());
		return Builder.Build();
	}
}
