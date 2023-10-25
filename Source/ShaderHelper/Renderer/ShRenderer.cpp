#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuApiInterface.h"

namespace SH
{
	const FString ShRenderer::DefaultVertexShaderText = R"(
		void MainVS(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position,
		out float2 Tex : TexCoord0
		)
		{
			Tex = float2(uint2(VertID, VertID << 1) & 2);
			Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
		}
	)";

	const FString ShRenderer::DefaultPixelShaderInput = 
R"(
struct PIn
{
	float4 Pos : SV_Position;
	float2 fragCoord : TexCoord0;
};
)";

	const FString ShRenderer::DefaultPixelShaderText = 
R"(float4 MainPS(PIn Input) : SV_Target
{
    return float4(Input.fragCoord.xy,0,1);
})";

	ShRenderer::ShRenderer()
		: ViewPort(nullptr)
	{

		VertexShader = GpuApi::CreateShaderFromSource(ShaderType::VertexShader, DefaultVertexShaderText, TEXT("DefaultFullScreenVS"), TEXT("MainVS"));
		FString ErrorInfo;
		check(GpuApi::CrossCompileShader(VertexShader, ErrorInfo));

		TUniquePtr<UniformBuffer> NewBuiltInUniformBuffer = UniformBufferBuilder{ "BuiltIn", UniformBufferUsage::Persistant }
			.AddVector2f("iResolution")
			.AddFloat("iTime")
			.AddVector2f("iMouse")
			.Build();
		BuiltInUniformBuffer = AUX::TransOwnerShip(MoveTemp(NewBuiltInUniformBuffer));


		auto [NewArgumentBuffer, NewArgumentBufferLayout] = ArgumentBufferBuilder{ 0 }
			.AddUniformBuffer(BuiltInUniformBuffer)
			.Build();

		BuiltInArgumentBuffer = MoveTemp(NewArgumentBuffer);
		BuiltInArgumentBufferLayout = MoveTemp(NewArgumentBufferLayout);
	}

	void ShRenderer::OnViewportResize()
	{
		check(ViewPort);
		//Note: BGRA8_UNORM is default framebuffer format in ue standalone renderer framework.
		GpuTextureDesc Desc{ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget };
		FinalRT = GpuApi::CreateGpuTexture(Desc);
		ViewPort->SetViewPortRenderTexture(FinalRT);
		ReCreatePipelineState();
		Render();
	}

	void ShRenderer::UpdatePixelShader(TRefCountPtr<GpuShader> NewPixelShader)
	{
		PixelShader = MoveTemp(NewPixelShader);
		if (FinalRT.IsValid()) {
			ReCreatePipelineState();
		}
	}

	FString ShRenderer::GetResourceDeclaration() const
	{
		return BuiltInArgumentBufferLayout->GetDeclaration();
	}

	void ShRenderer::ReCreatePipelineState()
	{
		check(VertexShader->IsCompiled());
		check(PixelShader->IsCompiled());
		PipelineStateDesc PipelineDesc{
			VertexShader, PixelShader, 
			RasterizerStateDesc{RasterizerFillMode::Solid, RasterizerCullMode::None}, 
			GpuResourceHelper::GDefaultBlendStateDesc,
			{ FinalRT->GetFormat() },
			BuiltInArgumentBufferLayout->GetBindLayout()
		};
		PipelineState = GpuApi::CreateRenderPipelineState(PipelineDesc);
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
        
		//Start to record command buffer
		if (FinalRT.IsValid())
		{
            GpuRenderPassDesc FullScreenPassDesc;
            FullScreenPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{FinalRT, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store});
            
            GpuApi::BeginRenderPass(FullScreenPassDesc, TEXT("FullScreenPass"));
			GpuApi::SetVertexBuffer(nullptr);
			GpuApi::SetRenderPipelineState(PipelineState);
			GpuApi::SetViewPort({ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y });

			BuiltInUniformBuffer->GetMember<float>("iTime") = iTime;
			GpuApi::SetBindGroups(BuiltInArgumentBuffer->GetBindGroup(), nullptr, nullptr, nullptr);
			GpuApi::DrawPrimitive(0, 3, 0, 1);
            GpuApi::EndRenderPass();

			//To make sure finalRT finished drawing
			GpuApi::FlushGpu();
			ViewPort->UpdateViewPortRenderTexture(FinalRT);
		}
	}

	void ShRenderer::RenderEnd()
	{
        Renderer::RenderEnd();
	}

}
