#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuRhi.h"
#include "Common/Path/PathHelper.h"

using namespace FRAMEWORK;

namespace SH
{
	const FString DefaultPixelShaderBody = 
R"(void mainImage(out float4 fragColor, in float2 fragCoord)
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    fragColor = float4(col,1);
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

		FString DefaultShaderSource;
		FFileHelper::LoadFileToString(DefaultShaderSource, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
		DefaultShaderSource = BuiltInBindGroupLayout->GetCodegenDeclaration() + DefaultShaderSource + DefaultPixelShaderBody;

		VertexShader = GGpuRhi->CreateShaderFromSource(ShaderType::VertexShader, DefaultShaderSource, TEXT("DefaultFullScreenVS"), TEXT("MainVS"));
		FString ErrorInfo;
		GGpuRhi->CrossCompileShader(VertexShader, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		PixelShader = GGpuRhi->CreateShaderFromSource(ShaderType::PixelShader, DefaultShaderSource, TEXT("DefaultFullScreenPS"), TEXT("MainPS"));
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
		GGpuRhi->SetResourceName("FinalRT", FinalRT);
		ReCreatePipelineState();
		Render();
	}

	void ShRenderer::UpdatePixelShader(TRefCountPtr<GpuShader> InNewPixelShader)
	{
		PixelShader = MoveTemp(InNewPixelShader);
		ReCreatePipelineState();
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
		GpuRenderPipelineStateDesc PipelineDesc{
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

		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(FullScreenPassDesc, TEXT("FullScreenPass"));
			{
				PassRecorder->SetRenderPipelineState(PipelineState);
				PassRecorder->SetBindGroups(BuiltInBindGroup, CustomBindGroup, nullptr, nullptr);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}

	void ShRenderer::RenderEnd()
	{
        Renderer::RenderEnd();
	}

}
