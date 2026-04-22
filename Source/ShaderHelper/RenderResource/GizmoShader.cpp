#include "CommonHeader.h"
#include "GizmoShader.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{
	GizmoShader::GizmoShader(const std::set<FString>& VariantDefinitions)
	{
		GpuBindGroupLayoutBuilder LayoutBuilder{BindingContext::PassSlot};
		UbBuilder.AddMatrix4x4f(TEXT("ViewProjection"));
		UbBuilder.AddVector3f(TEXT("GizmoCenter"));
		UbBuilder.AddFloat(TEXT("GizmoScale"));
		UbBuilder.AddInt(TEXT("HighlightAxis"));
		UbBuilder.AddMatrix4x4f(TEXT("GizmoOrientation"));
		UbBuilder.AddVector3f(TEXT("CameraPos"));
		UbBuilder.AddVector2f(TEXT("ViewportSize"));
		LayoutBuilder.AddUniformBuffer(TEXT("GizmoUb"), UbBuilder, BindingShaderStage::Vertex);
		BindGroupLayout = LayoutBuilder.Build();

		FString ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL);

		auto CompileShader = [&](const FString& FileName, const TCHAR* EntryPoint, ShaderType Type) -> TRefCountPtr<GpuShader> {
			auto Shader = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / FileName,
				.Type = Type,
				.EntryPoint = EntryPoint,
				.ExtraDecl = ExtraDecl,
			});
			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
			return Shader;
		};

		MoveVs = CompileShader(TEXT("GizmoMove.hlsl"), TEXT("MoveVS"), ShaderType::Vertex);
		MoveArrowVs = CompileShader(TEXT("GizmoMove.hlsl"), TEXT("MoveArrowVS"), ShaderType::Vertex);
		MovePlaneVs = CompileShader(TEXT("GizmoMove.hlsl"), TEXT("MovePlaneVS"), ShaderType::Vertex);
		RotateVs = CompileShader(TEXT("GizmoRotate.hlsl"), TEXT("RotateVS"), ShaderType::Vertex);
		ScaleVs = CompileShader(TEXT("GizmoScale.hlsl"), TEXT("ScaleVS"), ShaderType::Vertex);
		ScaleCubeVs = CompileShader(TEXT("GizmoScale.hlsl"), TEXT("ScaleCubeVS"), ShaderType::Vertex);
		ScaleAllVs = CompileShader(TEXT("GizmoScale.hlsl"), TEXT("ScaleAllVS"), ShaderType::Vertex);

		Ps = CompileShader(TEXT("GizmoCommon.hlsl"), TEXT("MainPS"), ShaderType::Pixel);
	}

	TRefCountPtr<GpuBindGroup> GizmoShader::GetBindGroup(const FMatrix44f& ViewProjection,
		const Vector3f& GizmoCenter, float GizmoScale, int32 HighlightAxis,
		const FMatrix44f& GizmoOrientation, const Vector3f& CameraPos,
		const FVector2f& ViewportSize)
	{
		GpuBindGroupBuilder Builder{BindGroupLayout};
		TUniquePtr<UniformBuffer> Ub = UbBuilder.Build();
		Ub->GetMember<FMatrix44f>(TEXT("ViewProjection")) = ViewProjection;
		Ub->GetMember<Vector3f>(TEXT("GizmoCenter")) = GizmoCenter;
		Ub->GetMember<float>(TEXT("GizmoScale")) = GizmoScale;
		Ub->GetMember<int32>(TEXT("HighlightAxis")) = HighlightAxis;
		Ub->GetMember<FMatrix44f>(TEXT("GizmoOrientation")) = GizmoOrientation;
		Ub->GetMember<Vector3f>(TEXT("CameraPos")) = CameraPos;
		Ub->GetMember<FVector2f>(TEXT("ViewportSize")) = ViewportSize;
		Builder.SetUniformBuffer(TEXT("GizmoUb"), Ub->GetGpuResource());
		return Builder.Build();
	}
}
