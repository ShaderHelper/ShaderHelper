#pragma once
#include "GpuTexture.h"
#include "GpuSampler.h"

namespace FW
{
	class GpuCmdRecorder;
}

namespace FW::GpuResourceHelper
{
	FRAMEWORK_API GpuTexture* GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat, GpuTextureUsage InUsage = GpuTextureUsage::RenderTarget);
	FRAMEWORK_API GpuSampler* GetSampler(const GpuSamplerDesc& InDesc);
	FRAMEWORK_API void ClearRWResource(GpuCmdRecorder* CmdRecorder, GpuResource* InResource);
}
