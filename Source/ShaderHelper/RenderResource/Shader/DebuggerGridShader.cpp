#include "CommonHeader.h"
#include "DebuggerGridShader.h"
#include "GpuApi/GpuRhi.h"
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"

using namespace FW;

namespace SH
{

	DebuggerGridShader::DebuggerGridShader(const std::set<FString>& VariantDefinitions)
	{
		GridUbBuilder.AddVector2f("Offset")
				 .AddUint("Zoom")
				 .AddVector2f("MouseLoc");
		
		BindGroupLayout = GpuBindGroupLayoutBuilder{ BindingContext::ShaderSlot }
							.AddUniformBuffer("GridUb", GridUbBuilder.GetLayoutDeclaration(), BindingShaderStage::Pixel)
							.AddTexture("InputTex", BindingShaderStage::Pixel)
							.Build();
		
		Vs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Grid.hlsl",
			.Type = ShaderType::VertexShader,
			.EntryPoint = "MainVS",
			.ExtraDecl = BindGroupLayout->GetCodegenDeclaration()
		});

		Ps = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Grid.hlsl",
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
			.ExtraDecl = BindGroupLayout->GetCodegenDeclaration()
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());

		GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
		check(ErrorInfo.IsEmpty());
	}

	TRefCountPtr<GpuBindGroup> DebuggerGridShader::GetBindGroup(const Parameters& InParameters)
	{
		TUniquePtr<UniformBuffer> GridUb = GridUbBuilder.Build();
		GridUb->GetMember<uint32>("Zoom") = InParameters.Zoom;
		GridUb->GetMember<Vector2f>("Offset") = InParameters.Offset;
		GridUb->GetMember<Vector2f>("MouseLoc") = InParameters.MouseLoc;
		
		return GpuBindGroupBuilder{ BindGroupLayout }
				.SetUniformBuffer("GridUb", GridUb->GetGpuResource())
				.SetTexture("InputTex", InParameters.InputTex)
				.Build();
	}

}
