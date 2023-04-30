#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalCommandList.h"
#include "MetalMap.h"

namespace FRAMEWORK
{
namespace GpuApi
{
		
	void InitApiEnv()
	{
        InitMetalCore();
	}

	void FlushGpu()
	{

	}

	void StartRenderFrame()
	{
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer(), [](const mtlpp::CommandBuffer&){
            dispatch_semaphore_signal(CpuSyncGpuSemaphore);
        });
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

	void SetRenderPipelineState(RenderPipelineState* InPipelineState)
	{

	}

	void SetVertexBuffer(GpuBuffer* InVertexBuffer)
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

    void BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        mtlpp::RenderPassDescriptor RenderPassDesc = MapRenderPassDesc(PassDesc);
        GetCommandListContext()->SetRenderPassDesc(MoveTemp(RenderPassDesc));
    }

    void EndRenderPass()
    {
      
    }

}
}
