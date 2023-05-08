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
        mtlpp::BlitCommandEncoder GetBlitCommandEncoder() {
            //Return a new BlitCommandEncoder when we want to copy something every time.
            return CurrentCommandBuffer.BlitCommandEncoder();
        }
        id<MTLCommandBuffer> GetCommandBuffer() const { return CurrentCommandBuffer.GetPtr(); }
 
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc, const FString& PassName) {
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentRenderCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
            CurrentRenderCommandEncoder.GetPtr().label = [NSString stringWithUTF8String:TCHAR_TO_ANSI(*PassName)];
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
        mtlpp::CommandBuffer CurrentCommandBuffer;
        
        bool IsPipelineDirty = true;
        bool IsViewportDirty = true;
        bool IsVertexBufferDirty = true;
    };

    inline CommandListContext* GetCommandListContext() {
        return &TSingleton<CommandListContext>::Get();
    }
}
