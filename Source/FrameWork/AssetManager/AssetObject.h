#pragma once
#include "GpuApi/GpuApiInterface.h"

namespace FRAMEWORK::RenderResourceUtil
{
	extern FRAMEWORK_API TRefCountPtr<GpuTexture> GlobalBlackTex;

	FRAMEWORK_API void Init();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat);
}