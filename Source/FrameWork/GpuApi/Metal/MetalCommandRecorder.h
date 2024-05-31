#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalArgumentBuffer.h"

namespace FRAMEWORK
{

    class MtlStateCache
    {
    public:
        MtlStateCache();
        void ApplyDrawState(MTL::RenderCommandEncoder* RenderCommandEncoder);
        void Clear();
        
        void SetPipeline(MetalRenderPipelineState* InPipelineState);
        void SetVertexBuffer(MetalBuffer* InBuffer);
        void SetViewPort(MTL::Viewport InViewPort, MTL::ScissorRect InSissorRect);
        void SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3);
        
    public:
        bool IsPipelineDirty : 1;
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
        MTL::RenderPassDescriptor*  CurrentRenderPassDesc;
        
        MetalBindGroup* CurrentBindGroup0;
        MetalBindGroup* CurrentBindGroup1;
        MetalBindGroup* CurrentBindGroup2;
        MetalBindGroup* CurrentBindGroup3;
    };

    class MtlRenderPassRecorder : public GpuRenderPassRecorder
    {
    public:
        MtlRenderPassRecorder(MTLRenderCommandEncoderPtr InCmdEncoder, MtlStateCache& InStateCache)
            : CmdEncoder(MoveTemp(InCmdEncoder))
            , StateCache(InStateCache)
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
        MtlStateCache& StateCache;
    };

    class MtlCmdRecorder : public GpuCmdRecorder
    {
    public:
        MtlCmdRecorder(MTLCommandBufferPtr InCmdBuffer)
            : CmdBuffer(MoveTemp(InCmdBuffer))
        {}
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
        MtlStateCache StateCache;
        
    private:
        MTLCommandBufferPtr CmdBuffer;
        TArray<TUniquePtr<MtlRenderPassRecorder>> RenderPassRecorders;
    };
    
    TArray<TUniquePtr<MtlCmdRecorder>> GMtlCmdRecorderPool;
    inline void ClearSubmitted()
    {
        for(auto It = GMtlCmdRecorderPool.CreateIterator(); It; It++)
        {
            if((*It)->IsSubmitted) {
                It.RemoveCurrent();
            }
        }
    }

    class CommandListContext
    {
    public:
        CommandListContext();
        
    public:
        id<MTLRenderCommandEncoder> GetRenderCommandEncoder() const { return CurrentRenderCommandEncoder.GetPtr(); }
        mtlpp::BlitCommandEncoder GetBlitCommandEncoder() {
            //Return a new BlitCommandEncoder when we want to copy something every time.
            return CurrentCommandBuffer.BlitCommandEncoder();
        }
        id<MTLCommandBuffer> GetCommandBuffer() const { return CurrentCommandBuffer.GetPtr(); }
        //TODO: Manage CommandBuffer
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc, const FString& PassName) {
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentRenderCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
            CurrentRenderCommandEncoder.GetPtr().label = [NSString stringWithUTF8String:TCHAR_TO_ANSI(*PassName)];
        }

        
 
    };

}
