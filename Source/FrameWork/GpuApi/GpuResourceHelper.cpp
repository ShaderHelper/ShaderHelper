#include "CommonHeader.h"
#include "GpuRhi.h"

namespace FW::GpuResourceHelper
{

	TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat, GpuTextureUsage InUsage)
	{
		GpuTextureDesc Desc{ 1, 1, InFormat,  InUsage};
		return GGpuRhi->CreateTexture(MoveTemp(Desc));
	}

	GpuTexture* GetGlobalBlackTex()
	{
        TArray<uint8> RawData = {0,0,0,1};
		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::ShaderResource , RawData};
		static TRefCountPtr<GpuTexture> GlobalBlackTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackTex;
	}

}
