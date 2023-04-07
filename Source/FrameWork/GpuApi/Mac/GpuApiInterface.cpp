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

    void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
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

	void BindVertexBuffer(GpuBuffer* InVertexBuffer)
	{

	}

	void SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{

	}

	void SetRenderTarget(GpuTexture* InGpuTexture)
	{

	}

	void SetClearColorValue(Vector4f ClearColor)
	{

	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType)
	{

	}

	void Submit()
	{

	}

	void BeginGpuCapture(const FString& SavedFileName)
	{

	}

	void EndGpuCapture()
	{

	}

	void BeginCaptureEvent(const FString& EventName)
	{

	}

	void EndCpatureEvent()
	{

	}

	void* GetSharedHandle(GpuTexture* InGpuTexture)
	{

	}

}
}
