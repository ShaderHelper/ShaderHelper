#include "CommonHeader.h"
#include "MetalCommandList.h"
#include "MetalMap.h"

namespace FRAMEWORK
{
    void CommandListContext::PrepareDrawingEnv()
    {
        check(CurrentRenderCommandEncoder);
        if(IsPipelineDirty)
        {
            check(CurrentPipelineState);
            CurrentRenderCommandEncoder.SetRenderPipelineState(CurrentPipelineState->GetResource());
            MarkPipelineDirty(false);
        }
        
        if(IsViewportDirty)
        {
            check(CurrentViewport.IsValid());
            check(CurrentScissorRect.IsValid());
            CurrentRenderCommandEncoder.SetViewport(*CurrentViewport);
            CurrentRenderCommandEncoder.SetScissorRect(*CurrentScissorRect);
            MarkViewportDirty(false);
        }
        
        if(IsVertexBufferDirty && CurrentVertexBuffer)
        {
            
        }
    }

    void CommandListContext::ClearBinding()
    {
        CurrentPipelineState = nullptr;
        CurrentViewport.Reset();
        CurrentScissorRect.Reset();
    }

}
