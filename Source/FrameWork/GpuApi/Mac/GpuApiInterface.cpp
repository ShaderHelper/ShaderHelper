#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalCommandList.h"
#include "MetalMap.h"
#include "MetalTexture.h"
#include "MetalShader.h"

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
        return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc));
	}

    void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
	{

	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{
        return new MetalShader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName));
	}

	bool CompilerShader(GpuShader* InShader)
	{
        return FRAMEWORK::CompileShader(static_cast<MetalShader*>(InShader));
	}

	TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc)
	{
        TRefCountPtr<MetalShader> Vs = AUX::StaticCastRefCountPtr<MetalShader>(InPipelineStateDesc.Vs);
        TRefCountPtr<MetalShader> Ps = AUX::StaticCastRefCountPtr<MetalShader>(InPipelineStateDesc.Ps);
        
        mtlpp::RenderPipelineDescriptor PipelineDesc;
        PipelineDesc.SetVertexFunction(Vs->GetCompilationResult());
        PipelineDesc.SetFragmentFunction(Ps->GetCompilationResult());
        
        ns::AutoReleased<ns::Array<mtlpp::RenderPipelineColorAttachmentDescriptor>> ColorAttachments = PipelineDesc.GetColorAttachments();
        BlendStateDesc BlendDesc =InPipelineStateDesc.BlendState;
        const uint32 BlendRtNum = BlendDesc.RtDescs.Num();
        for(uint32 i = 0; i < BlendRtNum; i++)
        {
            BlendRenderTargetDesc BlendRtInfo = BlendDesc.RtDescs[i];
            ColorAttachments[i].SetBlendingEnabled(BlendRtInfo.BlendEnable);
            ColorAttachments[i].SetSourceRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.SrcFactor));
            ColorAttachments[i].SetSourceAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.SrcAlphaFactor));
            ColorAttachments[i].SetDestinationRgbBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.DestFactor));
            ColorAttachments[i].SetDestinationAlphaBlendFactor((mtlpp::BlendFactor)MapBlendFactor(BlendRtInfo.DestAlphaFactor));
            ColorAttachments[i].SetRgbBlendOperation((mtlpp::BlendOperation)MapBlendOp(BlendRtInfo.ColorOp));
            ColorAttachments[i].SetAlphaBlendOperation((mtlpp::BlendOperation)MapBlendOp(BlendRtInfo.AlphaOp));
            ColorAttachments[i].SetWriteMask((mtlpp::ColorWriteMask)MapWriteMask(BlendRtInfo.Mask));
        }
        
        const uint32 RtFormatNum = InPipelineStateDesc.RtFormats.Num();
        for(uint32 i = 0; i < RtFormatNum; i++)
        {
            ColorAttachments[i].SetPixelFormat((mtlpp::PixelFormat)MapTextureFormat(InPipelineStateDesc.RtFormats[i]));
        }

        ns::AutoReleasedError Err;
        mtlpp::RenderPipelineState PipelineState = GDevice.NewRenderPipelineState(PipelineDesc, (mtlpp::PipelineOption)(1 << 0) ,nullptr ,&Err);
        if (!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create render pipeline: %s"), ConvertOcError(Err.GetPtr()));
            //TDOO: fallback to default pipeline.
        }
        
        return new MetalPipelineState(MoveTemp(PipelineState));
	}

	void SetRenderPipelineState(RenderPipelineState* InPipelineState)
	{
        GetCommandListContext()->SetPipeline(static_cast<MetalPipelineState*>(InPipelineState));
        GetCommandListContext()->MarkPipelineDirty(true);
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
        GetCommandListContext()->PrepareDrawingEnv();
        GetCommandListContext()->GetCommandEncoder().Draw(MapPrimitiveType(InType), StartVertexLocation, VertexCount, InstanceCount, InstanceCount);
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
