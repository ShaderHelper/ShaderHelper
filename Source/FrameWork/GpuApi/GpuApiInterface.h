#pragma once
#include "GpuResource.h"
namespace FRAMEWORK
{
namespace GpuApi
{
	FRAMEWORK_API void InitApiEnv(); 

	//Wait for all gpu work to finish
	FRAMEWORK_API void FlushGpu();

	FRAMEWORK_API void StartRenderFrame();
	FRAMEWORK_API void EndRenderFrame();

	FRAMEWORK_API TRefCountPtr<GpuTextureResource> CreateGpuTexture(const GpuTextureDesc& InTexDesc);
	FRAMEWORK_API void* MapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture, GpuResourceMapMode InMapMode);
	FRAMEWORK_API void UnMapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture);
}
}
