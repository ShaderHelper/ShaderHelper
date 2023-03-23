#pragma once
#include "GpuResource.h"
namespace FRAMEWORK
{
namespace GpuApi
{
	FRAMEWORK_API void InitApiEnv(); 

	//Wait for all gpu work to finish
	FRAMEWORK_API void FlushGpu();

	FRAMEWORK_API void StartFrame();
	FRAMEWORK_API void EndFrame();

	FRAMEWORK_API TRefCountPtr<GpuTextureResource> CreateGpuTexture(const GpuTextureDesc& InTexDesc);
	FRAMEWORK_API void* MapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture, TextureMapMode InMapMode);
	FRAMEWORK_API void UnMapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture);
}
}
