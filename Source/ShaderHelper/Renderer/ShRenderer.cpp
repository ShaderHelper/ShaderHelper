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
			return float4(1,1,0,0);
		}
	)";
#else
#endif

	ShRenderer::ShRenderer() 
		: ViewPort(nullptr)
		, bCanGpuCapture(false)
	{
		GpuApi::InitApiEnv();

		GpuTextureDesc Desc{ 256, 256, GpuTextureFormat::R16G16B16A16_UNORM, GpuTextureUsage::Shared | GpuTextureUsage::RenderTarget, Vector4f{0.5f}};
		FinalRT = GpuApi::CreateGpuTexture(Desc);

		VertexShader = GpuApi::CreateShaderFromSource(ShaderType::VertexShader, FullScreenVsText, TEXT("DefaultFullScreenVS"));
		GpuApi::CompilerShader(VertexShader);

		PixelShader = GpuApi::CreateShaderFromSource(ShaderType::PixelShader, PsForTest, TEXT("FullScreenPS"));
		GpuApi::CompilerShader(PixelShader);

		PipelineStateDesc PipelineDesc{
			VertexShader, PixelShader, RasterizerStateDesc{RasterizerFillMode::Solid, RasterizerCullMode::Back}, GpuResourceHelper::GDefaultBlendStateDesc, FinalRT->GetFormat()
		};
		PipelineState = GpuApi::CreateRenderPipelineState(PipelineDesc);
	}

	void* ShRenderer::GetSharedHanldeFromFinalRT() const
	{
		return nullptr;
	}

	void ShRenderer::RenderBegin()
	{
		bCanGpuCapture = FSlateApplication::Get().GetModifierKeys().AreModifersDown(EModifierKey::Control | EModifierKey::Alt);
		if (bCanGpuCapture) {
			GpuApi::BeginGpuCapture(TEXT("GpuCapture"));
		}
		GpuApi::StartRenderFrame();
	}

	void ShRenderer::Render()
	{
		RenderBegin();
		//Start to record command buffer
		GpuApi::BindVertexBuffer(nullptr);
		GpuApi::BindRenderPipelineState(PipelineState);
		GpuApi::SetViewPort(GpuViewPortDesc{ 256,256 });
		GpuApi::SetRenderTarget(FinalRT);
		GpuApi::DrawPrimitive(0, 3, 0, 1);

		RenderEnd();
	}

	void ShRenderer::RenderEnd()
	{
		GpuApi::Submit();
		GpuApi::EndRenderFrame();
		if (bCanGpuCapture) {
			GpuApi::EndGpuCapture();
			//just capture one frame.
			bCanGpuCapture = false;
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Successfully captured the current frame."), TEXT("Message:"));
		}

		ViewPort->SetViewPortRenderTexture(FinalRT);
	}

}