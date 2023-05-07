#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalCommandList.h"
#include "MetalMap.h"
#include "MetalTexture.h"
#include "MetalShader.h"
#include "MetalBuffer.h"
#include "MetalPipeline.h"

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
        Submit();
        dispatch_semaphore_wait(CpuSyncGpuSemaphore, DISPATCH_TIME_FOREVER);
	}

	void StartRenderFrame()
	{
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer(), [](const mtlpp::CommandBuffer&){
            dispatch_semaphore_signal(CpuSyncGpuSemaphore);
        });
	}

	void EndRenderFrame()
	{
        dispatch_semaphore_wait(CpuSyncGpuSemaphore, DISPATCH_TIME_FOREVER);
	}

	TRefCountPtr<GpuTexture> CreateGpuTexture(const GpuTextureDesc& InTexDesc)
	{
        return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc));
	}

    void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
	{
        MetalTexture* Texture = static_cast<MetalTexture*>(InGpuTexture);
        if (InMapMode == GpuResourceMapMode::Read_Only)
        {
            const uint32 BytesPerTexel = GetTextureFormatByteSize(InGpuTexture->GetFormat());
            const uint64 UnpaddedSize = InGpuTexture->GetWidth() * InGpuTexture->GetHeight() * BytesPerTexel;
            TRefCountPtr<MetalBuffer> ReadBackBuffer = CreateMetalBuffer(UnpaddedSize);
            OutRowPitch = InGpuTexture->GetWidth() * BytesPerTexel;
            
            id<MTLBlitCommandEncoder> BlitCommandEncoder = GetCommandListContext()->GetBlitCommandEncoder();
            MTLOrigin Origin = MTLOriginMake(0, 0, 0);
            MTLSize Size = MTLSizeMake(InGpuTexture->GetWidth(), InGpuTexture->GetHeight(), 1);
       
            [BlitCommandEncoder copyFromTexture:Texture->GetResource() sourceSlice:0 sourceLevel:0 sourceOrigin:Origin sourceSize:Size toBuffer:ReadBackBuffer->GetResource() destinationOffset:0 destinationBytesPerRow:OutRowPitch destinationBytesPerImage:UnpaddedSize];
            
            FlushGpu();
            return ReadBackBuffer->GetContents();
        }
	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{
        return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName)));
	}

	bool CompilerShader(GpuShader* InShader)
	{
        return FRAMEWORK::CompileShader(static_cast<MetalShader*>(InShader));
	}

	TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc)
	{
        return AUX::StaticCastRefCountPtr<GpuPipelineState>(CreateMetalPipelineState(InPipelineStateDesc));
	}

	void SetRenderPipelineState(GpuPipelineState* InPipelineState)
	{
        GetCommandListContext()->SetPipeline(static_cast<MetalPipelineState*>(InPipelineState));
        GetCommandListContext()->MarkPipelineDirty(true);
	}

	void SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
        GetCommandListContext()->SetVertexBuffer(static_cast<MetalBuffer*>(InVertexBuffer));
        GetCommandListContext()->MarkVertexBufferDirty(true);
	}

	void SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
        mtlpp::Viewport Viewport{
            (double)InViewPortDesc.TopLeftX, (double)InViewPortDesc.TopLeftY,
            (double)InViewPortDesc.Width, (double)InViewPortDesc.Height,
            (double)InViewPortDesc.ZMin, (double)InViewPortDesc.ZMax};
        
        mtlpp::ScissorRect ScissorRect{0, 0, InViewPortDesc.Width, InViewPortDesc.Height};
        GetCommandListContext()->SetViewPort(MakeUnique<mtlpp::Viewport>(MoveTemp(Viewport)), MakeUnique<mtlpp::ScissorRect>(MoveTemp(ScissorRect)));
	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType)
	{
        GetCommandListContext()->PrepareDrawingEnv();
        id<MTLRenderCommandEncoder> RenderCommandEncoder = GetCommandListContext()->GetRenderCommandEncoder();
        [RenderCommandEncoder drawPrimitives:MapPrimitiveType(InType) vertexStart:StartVertexLocation vertexCount:VertexCount instanceCount:InstanceCount baseInstance:StartInstanceLocation];
	}

	void Submit()
	{
        id<MTLBlitCommandEncoder> BlitCommandEncoder = GetCommandListContext()->GetBlitCommandEncoder();
        [BlitCommandEncoder endEncoding];
        id<MTLCommandBuffer> CommandBuffer = GetCommandListContext()->GetCommandBuffer();
        [CommandBuffer commit];
        
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer(), [](const mtlpp::CommandBuffer&){
            dispatch_semaphore_signal(CpuSyncGpuSemaphore);
        });
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

    void BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        mtlpp::RenderPassDescriptor RenderPassDesc = MapRenderPassDesc(PassDesc);
        GetCommandListContext()->SetRenderPassDesc(MoveTemp(RenderPassDesc));
        GetCommandListContext()->ClearBinding();
    }

    void EndRenderPass()
    {
        id<MTLRenderCommandEncoder> RenderCommandEncoder = GetCommandListContext()->GetRenderCommandEncoder();
        [RenderCommandEncoder endEncoding];
    }

}
}
