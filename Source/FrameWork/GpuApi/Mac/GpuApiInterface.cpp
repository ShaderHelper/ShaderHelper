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

	void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode)
	{

	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{

	}

	bool CompilerShader(GpuShader* InShader)
	{

	}

	TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc)
	{

	}

	void BindRenderPipelineState(RenderPipelineState* InPipelineState)
	{

	}

	void BindVertexBuffer()
	{

	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{

	}

}
}