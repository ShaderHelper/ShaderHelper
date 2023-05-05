#include "CommonHeader.h"
#include "MetalCommandList.h"
#include "MetalMap.h"

namespace FRAMEWORK
{
    void CommandListContext::PrepareDrawingEnv()
    {
        check(CurrentCommandEncoder);
        if(IsPipelineDirty)
        {
            check(CurrentPipelineState);
            CurrentCommandEncoder.SetRenderPipelineState(CurrentPipelineState->GetResource());
            MarkPipelineDirty(false);
        }
        
        if(IsViewportDirty)
        {
            check(CurrentViewport.IsValid());
            check(CurrentScissorRect.IsValid());
            CurrentCommandEncoder.SetViewport(*CurrentViewport);
            CurrentCommandEncoder.SetScissorRect(*CurrentScissorRect);
            MarkViewportDirty(false);
        }
        
        if(IsVertexBufferDirty && CurrentVertexBuffer)
        {
            
        }
    }

    void CommandListContext::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, FRAMEWORK::PrimitiveType InType)
    {
        check(CurrentCommandEncoder);
        CurrentCommandEncoder.Draw((mtlpp::PrimitiveType)MapPrimitiveType(InType), StartVertexLocation, VertexCount, InstanceCount, StartInstanceLocation);
    }

    void CommandListContext::Submit()
    {
        check(CurrentCommandBuffer);
        CurrentCommandBuffer.Commit();
    }

    void CommandListContext::ClearBinding()
    {
        CurrentPipelineState = nullptr;
        CurrentViewport.Reset();
        CurrentScissorRect.Reset();
    }

}
