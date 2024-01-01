#pragma once

namespace FRAMEWORK::GpuResourceHelper
{
	FRAMEWORK_API TRefCountPtr<GpuTexture> GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat);

}