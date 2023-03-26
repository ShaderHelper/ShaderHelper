#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
namespace FRAMEWORK
{
namespace GpuApi
{
		
	void InitApiEnv()
	{

	}

	void FlushGpu()
	{

	}

	void StartRenderFrame()
	{

	}

	void EndRenderFrame()
	{

	}

	TRefCountPtr<GpuTextureResource> CreateGpuTexture(const GpuTextureDesc& InTexDesc)
	{

	}

	void* MapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture, GpuResourceMapMode InMapMode)
	{

	}

	void UnMapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{

	}

	bool CompilerShader(TRefCountPtr<GpuShader> InShader)
	{

	}

}
}