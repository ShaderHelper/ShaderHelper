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

	TRefCountPtr<GpuTexture> CreateGpuTexture(const GpuTextureDesc& InTexDesc)
	{

	}

	void* MapGpuTexture(TRefCountPtr<GpuTexture> InGpuTexture, GpuResourceMapMode InMapMode)
	{

	}

	void UnMapGpuTexture(TRefCountPtr<GpuTexture> InGpuTexture)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{

	}

	bool CompilerShader(TRefCountPtr<GpuShader> InShader)
	{

	}

	TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc)
	{

	}

	void SetRenderPipelineState(TRefCountPtr<RenderPipelineState> InPipelineState)
	{

	}

}
}