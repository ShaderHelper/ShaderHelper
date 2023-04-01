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
	{
		GpuApi::InitApiEnv();
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

		RenderEnd();
	}

	void ShRenderer::RenderEnd()
	{
		GpuApi::EndRenderFrame();
		if (bCanGpuCapture) {
			GpuApi::EndGpuCapture();
			//just capture one frame.
			bCanGpuCapture = false;
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Successfully captured the current frame."), TEXT("Message:"));
		}
	}

}