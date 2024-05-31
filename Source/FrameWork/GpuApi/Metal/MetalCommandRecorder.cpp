#include "CommonHeader.h"
#include "MetalCommandRecorder.h"
#include "MetalMap.h"
#include "MetalDevice.h"

namespace FRAMEWORK
{

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

    GpuRenderPassRecorder* MtlCmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        MTL::RenderCommandEncoder* Encoder= CmdBuffer->renderCommandEncoder(MapRenderPassDesc(PassDesc));
        Encoder->setLabel(FStringToNSString(PassName));
        Encoder->
        
        auto PassRecorder = MakeUnique<MtlRenderPassRecorder>(NS::RetainPtr(Encoder)));
        RenderPassRecorders.Add(MoveTemp(PassRecorder));
        return RenderPassRecorders.Last().Get();
    }

    void MtlCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
    {
        MtlRenderPassRecorder* PassRecorder = static_cast<MtlRenderPassRecorder*>(InRenderPassRecorder);
        PassRecorder->GetEncoder()->endEncoding();
    }
}
