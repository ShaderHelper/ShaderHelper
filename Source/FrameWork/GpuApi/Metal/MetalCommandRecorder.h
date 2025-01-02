#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalArgumentBuffer.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{

    class MtlStateCache
    {
    public:
        MtlStateCache(MTLRenderPassDescriptorPtr InRenderPassDesc);
        void ApplyDrawState(MTL::RenderCommandEncoder* RenderCommandEncoder);
        void Clear();
        
        void SetPipeline(MetalRenderPipelineState* InPipelineState);
        void SetVertexBuffer(MetalBuffer* InBuffer);
        void SetViewPort(MTL::Viewport InViewPort, MTL::ScissorRect InSissorRect);
        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3);
        MTL::PrimitiveType GetPrimitiveType() const {
            check(CurrentRenderPipelineState);
            return (MTL::PrimitiveType)CurrentRenderPipelineState->GetPrimitiveType();
        }
        
    public:
        bool IsRenderPipelineDirty : 1;
        bool IsViewportDirty : 1;
        bool IsVertexBufferDirty : 1;
       
        bool IsBindGroup0Dirty : 1;
        bool IsBindGroup1Dirty : 1;
        bool IsBindGroup2Dirty : 1;
        bool IsBindGroup3Dirty : 1;
        
    private:
        MetalRenderPipelineState* CurrentRenderPipelineState;
        MetalBuffer* CurrentVertexBuffer;
        TOptional<MTL::Viewport> CurrentViewPort;
        TOptional<MTL::ScissorRect> CurrentScissorRect;
        
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
        
        MTLRenderPassDescriptorPtr RenderPassDesc;
    };

    class MtlRenderPassRecorder : public GpuRenderPassRecorder
    {
    public:
        MtlRenderPassRecorder(MTLRenderCommandEncoderPtr InCmdEncoder, MTLRenderPassDescriptorPtr InRenderPassDesc)
            : CmdEncoder(MoveTemp(InCmdEncoder))
            , StateCache(MoveTemp(InRenderPassDesc))
        {}
        
        MTL::RenderCommandEncoder* GetEncoder() const { return CmdEncoder.get(); }
        
    public:
        void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
        void SetRenderPipelineState(GpuPipelineState* InPipelineState) override;
        void SetVertexBuffer(GpuBuffer* InVertexBuffer) override;
        void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
        void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;
        
    private:
        MTLRenderCommandEncoderPtr CmdEncoder;
        MtlStateCache StateCache;
    };

    class MtlCmdRecorder : public GpuCmdRecorder
    {
    public:
        MtlCmdRecorder(MTLCommandBufferPtr InCmdBuffer)
            : CmdBuffer(MoveTemp(InCmdBuffer))
        {}
        
        MTL::CommandBuffer* GetCommandBuffer() const { return CmdBuffer.get(); }
        
    public:
        GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) override;
        void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) override;
        void BeginCaptureEvent(const FString& EventName) override;
        void EndCaptureEvent() override;
        void Barrier(GpuTrackedResource* InResource, GpuResourceState NewState) override;
        void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override;
        void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override;
        
    public:
        bool IsSubmitted{};
        
    private:
        MTLCommandBufferPtr CmdBuffer;
        TArray<TUniquePtr<MtlRenderPassRecorder>> RenderPassRecorders;
    };
    
    inline MtlCmdRecorder* LastSubmittedCmdRecorder;
    inline TArray<TUniquePtr<MtlCmdRecorder>> GMtlCmdRecorderPool;
    inline void ClearSubmitted()
    {
        for(auto It = GMtlCmdRecorderPool.CreateIterator(); It; It++)
        {
            if((*It)->IsSubmitted) {
                It.RemoveCurrent();
            }
        }
    }
 
}
