#include "CommonHeader.h"
#include "MetalCommandList.h"
#include "MetalMap.h"

namespace FRAMEWORK
{
    CommandListContext::CommandListContext()
        : CurrentPipelineState(nullptr)
        , CurrentVertexBuffer(nullptr)
        , CurrentBindGroup0(nullptr)
        , CurrentBindGroup1(nullptr)
        , CurrentBindGroup2(nullptr)
        , CurrentBindGroup3(nullptr)
        , IsPipelineDirty(false)
        , IsViewportDirty(false)
        , IsVertexBufferDirty(false)
        , IsBindGroupsDirty(false)
    {
        
    }

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
        
        if(IsBindGroupsDirty)
        {
            if(CurrentBindGroup0) { CurrentBindGroup0->Apply(GetRenderCommandEncoder()); }
            if(CurrentBindGroup1) { CurrentBindGroup1->Apply(GetRenderCommandEncoder()); }
            if(CurrentBindGroup2) { CurrentBindGroup2->Apply(GetRenderCommandEncoder()); }
            if(CurrentBindGroup3) { CurrentBindGroup3->Apply(GetRenderCommandEncoder()); }
            MarkBindGroupsDirty(false);
        }
    }

    void CommandListContext::ClearBinding()
    {
        CurrentPipelineState = nullptr;
        CurrentViewport.Reset();
        CurrentScissorRect.Reset();
        
        CurrentBindGroup0 = nullptr;
        CurrentBindGroup1 = nullptr;
        CurrentBindGroup2 = nullptr;
        CurrentBindGroup3 = nullptr;
        
        IsPipelineDirty = false;
        IsViewportDirty = false;
        IsVertexBufferDirty = false;
        IsBindGroupsDirty = false;
    }

}
