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
        , IsBindGroup0Dirty(false)
		, IsBindGroup1Dirty(false)
		, IsBindGroup2Dirty(false)
		, IsBindGroup3Dirty(false)
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
        
        if(CurrentBindGroup0 && IsBindGroup0Dirty) 
		{ 
			CurrentBindGroup0->Apply(GetRenderCommandEncoder()); 
			MarkBindGroup0Dirty(false);
		}
        if(CurrentBindGroup1 && IsBindGroup1Dirty)
		{ 
			CurrentBindGroup1->Apply(GetRenderCommandEncoder()); 
			MarkBindGroup1Dirty(false);
		}
		if(CurrentBindGroup2 && IsBindGroup2Dirty)
		{ 
			CurrentBindGroup2->Apply(GetRenderCommandEncoder());
			MarkBindGroup2Dirty(false);
		}
        if(CurrentBindGroup3 && IsBindGroup3Dirty)
		{ 
			CurrentBindGroup3->Apply(GetRenderCommandEncoder());
			MarkBindGroup3Dirty(false);
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
        
    }

}
