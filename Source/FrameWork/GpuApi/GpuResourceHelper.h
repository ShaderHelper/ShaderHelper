#pragma once

namespace FRAMEWORK::GpuResourceHelper
{
	FRAMEWORK_API GpuTexture* GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat);

}