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
        if(!CurrentViewPort.IsSet())
        {
            auto ColorAttachments = CurrentRenderPassDesc.GetColorAttachments();
            mtlpp::Texture Rt = ColorAttachments[0].GetTexture();
            if(Rt)
            {
                mtlpp::Viewport Viewport{
                    0, 0,
                    (double)Rt.GetWidth(), (double)Rt.GetHeight(),
                    0, 1.0
                };

                mtlpp::ScissorRect ScissorRect{ 0, 0, Rt.GetWidth(), Rt.GetHeight() };

                SetViewPort(MoveTemp(Viewport), MoveTemp(ScissorRect));
            }
        }
        
        if(IsPipelineDirty)
        {
            check(CurrentPipelineState);
            CurrentRenderCommandEncoder.SetRenderPipelineState(CurrentPipelineState->GetResource());
            MarkPipelineDirty(false);
        }
        
        if(IsViewportDirty)
        {
			if (CurrentViewPort)
			{
				CurrentRenderCommandEncoder.SetViewport(*CurrentViewPort);
				CurrentRenderCommandEncoder.SetScissorRect(*CurrentScissorRect);
				MarkViewportDirty(false);
			}
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

    void CommandListContext::Draw(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
    {
        check(CurrentPipelineState);
        [GetRenderCommandEncoder() drawPrimitives:CurrentPipelineState->GetPrimitiveType() vertexStart:StartVertexLocation vertexCount:VertexCount instanceCount:InstanceCount baseInstance:StartInstanceLocation];
    }

    void CommandListContext::ClearBinding()
    {
        CurrentPipelineState = nullptr;
        CurrentViewPort.Reset();
        CurrentScissorRect.Reset();
        
        CurrentBindGroup0 = nullptr;
        CurrentBindGroup1 = nullptr;
        CurrentBindGroup2 = nullptr;
        CurrentBindGroup3 = nullptr;
        
    }

}
