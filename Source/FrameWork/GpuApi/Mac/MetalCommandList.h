#pragma once
#include "MetalCommon.h"
#include "MetalPipeline.h"
#include "Common/Util/Singleton.h"

namespace FRAMEWORK
{
    class CommandListContext
    {
    public:
        CommandListContext() : CurrentPipelineState(nullptr) {}
    public:
        void SetCommandBuffer(mtlpp::CommandBuffer InCommandBuffer, mtlpp::CommandBufferHandler InHandler) {
            CurrentCommandBuffer = MoveTemp(InCommandBuffer);
            CurrentCommandBuffer.AddCompletedHandler(InHandler);
        }
        void SetRenderPassDesc(mtlpp::RenderPassDescriptor InPassDesc) {
            CurrentRenderPassDesc = MoveTemp(InPassDesc);
            CurrentCommandEncoder = CurrentCommandBuffer.RenderCommandEncoder(CurrentRenderPassDesc);
        }
        
        void SetPipeline(MetalPipelineState* InPipelineState) { CurrentPipelineState = InPipelineState; }
        
        const mtlpp::RenderCommandEncoder& GetCommandEncoder() const { return  CurrentCommandEncoder; }
        
        void PrepareDrawingEnv();
        
        void ClearBinding() {
            CurrentPipelineState = nullptr;
        }
        
        void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
        
    private:
        MetalPipelineState* CurrentPipelineState;
        
        
        mtlpp::RenderPassDescriptor  CurrentRenderPassDesc;
        mtlpp::RenderCommandEncoder CurrentCommandEncoder;
        mtlpp::CommandBuffer CurrentCommandBuffer;
        
        bool IsPipelineDirty = false;
    };

    inline CommandListContext* GetCommandListContext() {
        return &TSingleton<CommandListContext>::Get();
    }
}
