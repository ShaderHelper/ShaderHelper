#include "CommonHeader.h"
#include "RenderResourceUtil.h"

namespace FRAMEWORK::RenderResourceUtil
{
	TRefCountPtr<GpuTexture> GlobalBlackTex;

	void Init()
	{
		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::ShaderResource };
		GlobalBlackTex = GpuApi::CreateGpuTexture(MoveTemp(Desc));
	}

	TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat)
	{
		GpuTextureDesc Desc{ 1, 1, InFormat, GpuTextureUsage::RenderTarget };
		return GpuApi::CreateGpuTexture(MoveTemp(Desc));
	}

}