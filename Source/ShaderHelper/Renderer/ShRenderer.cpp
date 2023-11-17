#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuApiInterface.h"
#include "Renderer/RenderResource/RenderResourceUtil.h"

namespace SH
{
	const FString ShRenderer::DefaultVertexShaderText = R"(
		void MainVS(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position
		)
		{
			float2 uv = float2(uint2(VertID, VertID << 1) & 2);
			Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
		}
	)";

	const FString ShRenderer::DefaultPixelShaderInput = 
R"(
struct PIn
{
	float4 Pos : SV_Position;
};
)";

	const FString ShRenderer::DefaultPixelShaderMacro = 
R"(
#define fragCoord float2(Input.Pos.x, iResolution.y - Input.Pos.y)
)";

	const FString ShRenderer::DefaultPixelShaderText = 
R"(float4 MainPS(PIn Input) : SV_Target
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    return float4(col,1);
})";

	ShRenderer::ShRenderer(PreviewViewPort* InViewPort)
		: ViewPort(InViewPort)
	{

		VertexShader = GpuApi::CreateShaderFromSource(ShaderType::VertexShader, DefaultVertexShaderText, TEXT("DefaultFullScreenVS"), TEXT("MainVS"));
		FString ErrorInfo;
		check(GpuApi::CrossCompileShader(VertexShader, ErrorInfo));

		TUniquePtr<UniformBuffer> NewBuiltInUniformBuffer = UniformBufferBuilder{ "BuiltIn", UniformBufferUsage::Persistant }
			.AddVector2f("iResolution")
			.AddFloat("iTime")
			.Build();
		BuiltInUniformBuffer = AUX::TransOwnerShip(MoveTemp(NewBuiltInUniformBuffer));


		auto [NewArgumentBuffer, NewArgumentBufferLayout] = ArgumentBufferBuilder{ 0 }
			.AddUniformBuffer(BuiltInUniformBuffer, BindingShaderStage::Pixel)
			.Build();

		BuiltInArgumentBuffer = MoveTemp(NewArgumentBuffer);
		BuiltInArgumentBufferLayout = MoveTemp(NewArgumentBufferLayout);

		FinalRT = RenderResourceUtil::TempRenderTarget(GpuTextureFormat::B8G8R8A8_UNORM);
	}

	void ShRenderer::OnViewportResize()
	{
		check(ViewPort);
		iResolution = { (float)ViewPort->GetSize().X, (float)ViewPort->GetSize().Y };
		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		GpuTextureDesc Desc{ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		FinalRT = GpuApi::CreateGpuTexture(Desc);
		ViewPort->SetViewPortRenderTexture(FinalRT);
		ReCreatePipelineState();
		Render();
	}

	void ShRenderer::UpdatePixelShader(TRefCountPtr<GpuShader> InNewPixelShader)
	{
		NewPixelShader = MoveTemp(InNewPixelShader);
		bCompileSuccessful = NewPixelShader.IsValid();
		if (bCompileSuccessful)
		{
			OldPixelShader = NewPixelShader;
			OldCustomArgumentBuffer = NewCustomArgumentBuffer;
			OldCustomArgumentBufferLayout = NewCustomArgumentBufferLayout;
			ReCreatePipelineState();
			OldPipelineState = NewPipelineState;
		}
	}

	FString ShRenderer::GetResourceDeclaration() const
	{
		FString Declaration = BuiltInArgumentBufferLayout->GetDeclaration();
		if (NewCustomArgumentBufferLayout)
		{
			Declaration += NewCustomArgumentBufferLayout->GetDeclaration();
		}
		return Declaration;
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
		check(NewPixelShader->IsCompiled());
		PipelineStateDesc PipelineDesc{
				VertexShader, NewPixelShader,
				RasterizerStateDesc{ RasterizerFillMode::Solid, RasterizerCullMode::None },
				GpuResourceHelper::GDefaultBlendStateDesc,
				{ FinalRT->GetFormat() },
				BuiltInArgumentBufferLayout->GetBindLayout()
		};

		if (NewCustomArgumentBufferLayout.IsValid())
		{
			PipelineDesc.BindGroupLayout1 = NewCustomArgumentBufferLayout->GetBindLayout();
		}
	
		NewPipelineState = GpuApi::CreateRenderPipelineState(PipelineDesc);
	}

	void ShRenderer::RenderNewRenderPass()
	{
		GpuRenderPassDesc FullScreenPassDesc;
		FullScreenPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ FinalRT, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });
		GpuApi::BeginRenderPass(FullScreenPassDesc, TEXT("FullScreenPass"));
		GpuApi::SetRenderPipelineState(NewPipelineState);
		GpuApi::SetViewPort({ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y });
		if (NewCustomArgumentBuffer.IsValid())
		{
			GpuApi::SetBindGroups(BuiltInArgumentBuffer->GetBindGroup(), NewCustomArgumentBuffer->GetBindGroup(), nullptr, nullptr);
		}
		else
		{
			GpuApi::SetBindGroups(BuiltInArgumentBuffer->GetBindGroup(), nullptr, nullptr, nullptr);
		}
		GpuApi::DrawPrimitive(0, 3, 0, 1);
		GpuApi::EndRenderPass();
	}

	void ShRenderer::RenderOldRenderPass()
	{
		GpuRenderPassDesc FullScreenPassDesc;
		FullScreenPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ FinalRT, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });
		GpuApi::BeginRenderPass(FullScreenPassDesc, TEXT("FullScreenPass"));
		GpuApi::SetRenderPipelineState(OldPipelineState);
		GpuApi::SetViewPort({ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y });
		if (OldCustomArgumentBuffer.IsValid())
		{
			GpuApi::SetBindGroups(BuiltInArgumentBuffer->GetBindGroup(), OldCustomArgumentBuffer->GetBindGroup(), nullptr, nullptr);
		}
		else
		{
			GpuApi::SetBindGroups(BuiltInArgumentBuffer->GetBindGroup(), nullptr, nullptr, nullptr);
		}
		GpuApi::DrawPrimitive(0, 3, 0, 1);
		GpuApi::EndRenderPass();
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
		if (bCompileSuccessful)
		{
			RenderNewRenderPass();
		}
		else
		{
			RenderOldRenderPass();
		}
	}

	void ShRenderer::RenderEnd()
	{
        Renderer::RenderEnd();
	}

}
