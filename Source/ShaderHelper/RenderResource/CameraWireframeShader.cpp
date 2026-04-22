#include "CommonHeader.h"
#include "CameraWireframeShader.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	CameraWireframeShader::CameraWireframeShader(const std::set<FString>& VariantDefinitions)
	{
		GpuBindGroupLayoutBuilder LayoutBuilder{BindingContext::PassSlot};
		UbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
		UbBuilder.AddMatrix4x4f(TEXT("InvViewProjection"));
		UbBuilder.AddVector4f(TEXT("WireColor"));
		LayoutBuilder.AddUniformBuffer(TEXT("CameraWireUb"), UbBuilder, BindingShaderStage::Vertex);
		BindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "CameraWireframe.hlsl",
			.Type = ShaderType::Vertex,
			.EntryPoint = "MainVS",
			.ExtraDecl = ExtraDecl,
		});
		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "CameraWireframe.hlsl",
			.Type = ShaderType::Pixel,
			.EntryPoint = "MainPS",
			.ExtraDecl = ExtraDecl,
		});
		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> CameraWireframeShader::GetBindGroup(const FMatrix44f& ViewProjection,
		const FMatrix44f& InvViewProjection, const FVector4f& WireColor)
	{
		GpuBindGroupBuilder Builder{BindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<FMatrix44f>(TEXT("InvViewProjection")) = InvViewProjection;
		Ub->GetMember<FVector4f>(TEXT("WireColor")) = WireColor;
		Builder.SetUniformBuffer(TEXT("CameraWireUb"), Ub->GetGpuResource());
		return Builder.Build();
	}
}
