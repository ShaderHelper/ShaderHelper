#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuApiInterface.h"
namespace SH
{
#if PLATFORM_WINDOWS
	const FString FullScreenVsText = R"(
		void main(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position,
		out float2 Tex : TexCoord0
		)
		{
			Tex = float2(uint2(VertID, VertID << 1) & 2);
			Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
		}
	)";

	const FString PsForTest = R"(
		float4 main() : SV_Target 
		{
			return float4(1,1,0,1);
		}
	)";
#else
#endif

	ShRenderer::ShRenderer() 
		: ViewPort(nullptr)
		, bCanGpuCapture(false)
	{
		GpuApi::InitApiEnv();

		VertexShader = GpuApi::CreateShaderFromSource(ShaderType::VertexShader, FullScreenVsText, TEXT("DefaultFullScreenVS"));
		GpuApi::CompilerShader(VertexShader);

		PixelShader = GpuApi::CreateShaderFromSource(ShaderType::PixelShader, PsForTest, TEXT("FullScreenPS"));
		GpuApi::CompilerShader(PixelShader);
	}

	void ShRenderer::OnViewportResize()
	{
		check(ViewPort);
		GpuTextureDesc Desc{ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::Shared | GpuTextureUsage::RenderTarget };
		FinalRT = GpuApi::CreateGpuTexture(Desc);
		ViewPort->SetViewPortRenderTexture(FinalRT);

		check(FinalRT.IsValid());
		PipelineStateDesc PipelineDesc{
			VertexShader, PixelShader, RasterizerStateDesc{RasterizerFillMode::Solid, RasterizerCullMode::None}, GpuResourceHelper::GDefaultBlendStateDesc, FinalRT->GetFormat()
		};
		PipelineState = GpuApi::CreateRenderPipelineState(PipelineDesc);
	
		Render();
	}

	void ShRenderer::RenderBegin()
	{
		GpuApi::StartRenderFrame();
		bCanGpuCapture = FSlateApplication::Get().GetModifierKeys().AreModifersDown(EModifierKey::Control | EModifierKey::Alt);
		if (bCanGpuCapture) {
			GpuApi::BeginGpuCapture(TEXT("GpuCapture"));
		}
	}

	void ShRenderer::Render()
	{
		RenderBegin();
		//Start to record command buffer
		if (FinalRT.IsValid())
		{
			GpuApi::BeginCaptureEvent(TEXT("Full screen pass"));

			GpuApi::BindVertexBuffer(nullptr);
			GpuApi::BindRenderPipelineState(PipelineState);
			GpuApi::SetViewPort({ (uint32)ViewPort->GetSize().X, (uint32)ViewPort->GetSize().Y });
			GpuApi::SetRenderTarget(FinalRT);
			GpuApi::DrawPrimitive(0, 3, 0, 1);

			GpuApi::EndCpatureEvent();

			//To make sure finalRT finished drawing
			GpuApi::FlushGpu();
			ViewPort->UpdateViewPortRenderTexture(FinalRT);
		}

		RenderEnd();
	}

	void ShRenderer::RenderEnd()
	{
		if (bCanGpuCapture) {
			GpuApi::EndGpuCapture();
			//just capture one frame.
			bCanGpuCapture = false;
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Successfully captured the current frame."), TEXT("Message:"));
		}
		GpuApi::Submit();
		GpuApi::EndRenderFrame();

	}

}