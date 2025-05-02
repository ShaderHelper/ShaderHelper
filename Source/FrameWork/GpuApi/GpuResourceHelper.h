#pragma once

namespace FW
{
	class GpuCmdRecorder;
}

namespace FW::GpuResourceHelper
{
	FRAMEWORK_API GpuTexture* GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat, GpuTextureUsage InUsage = GpuTextureUsage::RenderTarget);
	
	FRAMEWORK_API void ClearRWResource(GpuCmdRecorder* CmdRecorder, GpuResource* InResource);
}
