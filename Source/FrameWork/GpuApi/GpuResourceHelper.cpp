#include "CommonHeader.h"
#include "GpuRhi.h"

namespace FW::GpuResourceHelper
{

	TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat)
	{
		GpuTextureDesc Desc{ 1, 1, InFormat, GpuTextureUsage::RenderTarget };
		return GGpuRhi->CreateTexture(MoveTemp(Desc));
	}

	GpuTexture* GetGlobalBlackTex()
	{
		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::ShaderResource };
		static TRefCountPtr<GpuTexture> GlobalBlackTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackTex;
	}

}