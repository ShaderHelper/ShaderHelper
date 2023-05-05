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
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer, mtlpp::CommandBufferHandler InHandler) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
            CurrentCommandBuffer.AddCompletedHandler(InHandler);
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc) {
            CurrentCommandEncoder.EndEncoding();
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
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
        
    public:
        void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType);
        void Submit();
        
    private:
        MetalPipelineState* CurrentPipelineState;
        MetalBuffer* CurrentVertexBuffer;
        TUniquePtr<mtlpp::Viewport> CurrentViewport;
        TUniquePtr<mtlpp::ScissorRect> CurrentScissorRect;
        
        mtlpp::RenderPassDescriptor  CurrentRenderPassDesc;
        mtlpp::RenderCommandEncoder CurrentCommandEncoder;
        mtlpp::CommandBuffer CurrentCommandBuffer;
        
        bool IsPipelineDirty = true;
        bool IsViewportDirty = true;
        bool IsVertexBufferDirty = true;
    };

    inline CommandListContext* GetCommandListContext() {
        return &TSingleton<CommandListContext>::Get();
    }
}
