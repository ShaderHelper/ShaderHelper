#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuRhi.h"

using namespace FRAMEWORK;

namespace SH
{
	const FString DefaultVertexShaderText = R"(
		void MainVS(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position
		)
		{
			float2 uv = float2(uint2(VertID, VertID << 1) & 2);
			Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
		}
	)";

	const FString DefaultPixelShaderInput = 
R"(
struct PIn
{
	float4 Pos : SV_Position;
};
)";

	const FString DefaultPixelShaderMacro = 
R"(
#define fragCoord float2(Input.Pos.x, iResolution.y - Input.Pos.y)
)";

	const FString DefaultPixelShaderBody = 
R"(float4 MainPS(PIn Input) : SV_Target
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    return float4(col,1);
})";

	ShRenderer::ShRenderer()
		: iTime(0)
		, iResolution{0,0}
	{

		BuiltInUniformBuffer = UniformBufferBuilder{ UniformBufferUsage::Persistant }
								.AddVector2f("iResolution")
								.AddFloat("iTime")
								.Build();

		BuiltInBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
								.AddUniformBuffer("BuiltIn", BuiltInUniformBuffer->GetDeclaration(), BindingShaderStage::Pixel)
								.Build();

		BuiltInBindGroup = GpuBindGrouprBuilder{ BuiltInBindGroupLayout }
							.SetUniformBuffer("BuiltIn", BuiltInUniformBuffer->GetGpuResource())
							.Build();

		VertexShader = GGpuRhi->CreateShaderFromSource(ShaderType::VertexShader, DefaultVertexShaderText, TEXT("DefaultFullScreenVS"), TEXT("MainVS"));
		FString ErrorInfo;
		GGpuRhi->CrossCompileShader(VertexShader, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		PixelShader = GGpuRhi->CreateShaderFromSource(ShaderType::PixelShader, GetPixelShaderDeclaration() + DefaultPixelShaderBody, TEXT("DefaultFullScreenPS"), TEXT("MainPS"));
		GGpuRhi->CrossCompileShader(PixelShader, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		FinalRT = GpuResourceHelper::TempRenderTarget(GpuTextureFormat::B8G8R8A8_UNORM);

		ReCreatePipelineState();
	}

	void ShRenderer::OnViewportResize(const Vector2f& InResolution)
	{
		iResolution = InResolution;
		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		GpuTextureDesc Desc{ (uint32)iResolution.x, (uint32)iResolution.y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		FinalRT = GGpuRhi->CreateTexture(Desc);
		GGpuRhi->SetTextureName("FinalRT", FinalRT);
		ReCreatePipelineState();
		Render();
	}

	void ShRenderer::UpdatePixelShader(TRefCountPtr<GpuShader> InNewPixelShader)
	{
		PixelShader = MoveTemp(InNewPixelShader);
		ReCreatePipelineState();
	}

	FString ShRenderer::GetPixelShaderDeclaration() const
	{
		FString Declaration = BuiltInBindGroupLayout->GetCodegenDeclaration();
		if (CustomBindGroupLayout)
		{
			Declaration += CustomBindGroupLayout->GetCodegenDeclaration();
		}
		return DefaultPixelShaderInput + Declaration + DefaultPixelShaderMacro;
	}

	FString ShRenderer::GetDefaultPixelShaderBody() const
	{
		return DefaultPixelShaderBody;
	}

	TArray<TSharedRef<PropertyData>> ShRenderer::GetBuiltInPropertyDatas() const
	{
		TArray<TSharedRef<PropertyData>> Datas;

		TSharedRef<PropertyCategory> BuiltInCategory = MakeShared<PropertyCategory>("Built In");
		TSharedRef<PropertyCategory> UniformSubCategory = MakeShared<PropertyCategory>("Uniform");
		BuiltInCategory->AddChild(UniformSubCategory);

		auto iTimeProperty = MakeShared<PropertyItem<float>>("iTime", TAttribute<float>::CreateLambda([this] { return iTime; }));
		iTimeProperty->SetEnabled(false);

		auto iResolutionProperty = MakeShared<PropertyItem<Vector2f>>("iResolution", TAttribute<Vector2f>::CreateLambda([this] {return iResolution; }));
		iResolutionProperty->SetEnabled(false);

		UniformSubCategory->AddChild(MoveTemp(iTimeProperty));
		UniformSubCategory->AddChild(MoveTemp(iResolutionProperty));

		Datas.Add(MoveTemp(BuiltInCategory));
		return Datas;
	}

	void ShRenderer::ReCreatePipelineState()
	{
		check(VertexShader->IsCompiled());
		check(PixelShader->IsCompiled());
		GpuPipelineStateDesc PipelineDesc{
			VertexShader, PixelShader,
			{ 
				{  FinalRT->GetFormat() }
			},
			{ BuiltInBindGroupLayout, CustomBindGroupLayout }
		};	
		PipelineState = GGpuRhi->CreateRenderPipelineState(PipelineDesc);
	}

	void ShRenderer::RenderBegin()
	{
        Renderer::RenderBegin();
		static double StartRenderTime = FPlatformTime::Seconds();
		iTime = float(FPlatformTime::Seconds() - StartRenderTime);
	}

	void ShRenderer::RenderInternal()
	{
        Renderer::RenderInternal();
        
		BuiltInUniformBuffer->GetMember<float>("iTime") = iTime;
		BuiltInUniformBuffer->GetMember<Vector2f>("iResolution") = iResolution;

		//Start to record command buffer
		GpuRenderPassDesc FullScreenPassDesc;
		FullScreenPassDesc.ColorRenderTargets = {
			{ FinalRT, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store },
		};
		GGpuRhi->BeginRenderPass(FullScreenPassDesc, TEXT("FullScreenPass"));
		{
			GGpuRhi->SetRenderPipelineState(PipelineState);
			GGpuRhi->SetBindGroups(BuiltInBindGroup, CustomBindGroup, nullptr, nullptr);
			GGpuRhi->DrawPrimitive(0, 3, 0, 1);
		}
		GGpuRhi->EndRenderPass();
		
		GGpuRhi->Submit();
	}

	void ShRenderer::RenderEnd()
	{
        Renderer::RenderEnd();
	}

}
