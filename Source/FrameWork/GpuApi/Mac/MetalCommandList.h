#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "Common/Util/Singleton.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"

namespace FRAMEWORK
{
    class CommandListContext
    {
    public:
        CommandListContext() : CurrentPipelineState(nullptr) , CurrentVertexBuffer(nullptr) {}
        
    public:
        id<MTLRenderCommandEncoder> GetRenderCommandEncoder() const { return CurrentRenderCommandEncoder.GetPtr(); }
        id<MTLBlitCommandEncoder> GetBlitCommandEncoder() const { return CurrentBlitCommandEncoder.GetPtr(); }
        id<MTLCommandBuffer> GetCommandBuffer() const { return CurrentCommandBuffer.GetPtr(); }
 
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer, mtlpp::CommandBufferHandler InHandler) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
            CurrentCommandBuffer.AddCompletedHandler(InHandler);
            CurrentBlitCommandEncoder = CurrentCommandBuffer.BlitCommandEncoder();
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc) {
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentRenderCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
        }
        void SetPipeline(MetalPipelineState* InPipelineState) { CurrentPipelineState = InPipelineState; }
        void SetVertexBuffer(MetalBuffer* InBuffer) { CurrentVertexBuffer = InBuffer; }
        void SetViewPort(TUniquePtr<mtlpp::Viewport> InViewPort, TUniquePtr<mtlpp::ScissorRect> InSissorRect) {
            CurrentViewport = MoveTemp(InViewPort);
            CurrentScissorRect = MoveTemp(InSissorRect);
        }
        
        void PrepareDrawingEnv();
        void ClearBinding();
        
        void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
        void MarkViewportDirty(bool IsDirty) { IsViewportDirty = IsDirty; }
        void MarkVertexBufferDirty(bool IsDirty) { IsVertexBufferDirty = IsDirty; }
        
    private:
        MetalPipelineState* CurrentPipelineState;
        MetalBuffer* CurrentVertexBuffer;
        TUniquePtr<mtlpp::Viewport> CurrentViewport;
        TUniquePtr<mtlpp::ScissorRect> CurrentScissorRect;
        
        mtlpp::RenderPassDescriptor  CurrentRenderPassDesc;
        mtlpp::RenderCommandEncoder CurrentRenderCommandEncoder;
        mtlpp::BlitCommandEncoder CurrentBlitCommandEncoder;
        mtlpp::CommandBuffer CurrentCommandBuffer;
        
        bool IsPipelineDirty = true;
        bool IsViewportDirty = true;
        bool IsVertexBufferDirty = true;
    };

    inline CommandListContext* GetCommandListContext() {
        return &TSingleton<CommandListContext>::Get();
    }
}
