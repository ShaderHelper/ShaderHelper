#pragma once

namespace FW::GpuResourceHelper
{
	FRAMEWORK_API GpuTexture* GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat, GpuTextureUsage InUsage = GpuTextureUsage::RenderTarget);

}
