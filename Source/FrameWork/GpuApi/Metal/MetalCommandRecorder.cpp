#include "CommonHeader.h"
#include "MetalCommandRecorder.h"
#include "MetalMap.h"
#include "MetalDevice.h"

namespace FW
{

    MtlStateCache::MtlStateCache(MTLRenderPassDescriptorPtr InRenderPassDesc)
        : CurrentRenderPipelineState(nullptr)
        , CurrentVertexBuffer(nullptr)
        , CurrentBindGroup0(nullptr)
        , CurrentBindGroup1(nullptr)
        , CurrentBindGroup2(nullptr)
        , CurrentBindGroup3(nullptr)
        , IsRenderPipelineDirty(false)
        , IsViewportDirty(false)
        , IsVertexBufferDirty(false)
        , IsBindGroup0Dirty(false)
        , IsBindGroup1Dirty(false)
        , IsBindGroup2Dirty(false)
        , IsBindGroup3Dirty(false)
        , RenderPassDesc(MoveTemp(InRenderPassDesc))
    {
        
    }

    void MtlStateCache::SetPipeline(MetalRenderPipelineState* InPipelineState)
    {
        if (InPipelineState != CurrentRenderPipelineState)
        {
            CurrentRenderPipelineState = InPipelineState;
            IsRenderPipelineDirty = true;
        }
    }

    void MtlStateCache::SetVertexBuffer(MetalBuffer* InBuffer)
    {
        if (CurrentVertexBuffer != InBuffer)
        {
            CurrentVertexBuffer = InBuffer;
            IsVertexBufferDirty = true;
        }
    }

    void MtlStateCache::SetViewPort(MTL::Viewport InViewPort, MTL::ScissorRect InSissorRect)
    {
        if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort , &InViewPort, sizeof(MTL::Viewport)))
        {
            CurrentViewPort = MoveTemp(InViewPort);
            CurrentScissorRect = MoveTemp(InSissorRect);
            IsViewportDirty = true;
        }
    }

    void MtlStateCache::SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3)
    {
        if (InGroup0 != CurrentBindGroup0) {
            CurrentBindGroup0 = InGroup0;
            IsBindGroup0Dirty = true;
        }
        if (InGroup1 != CurrentBindGroup1) {
            CurrentBindGroup1 = InGroup1;
            IsBindGroup1Dirty = true;
        }
        if (InGroup2 != CurrentBindGroup2) {
            CurrentBindGroup2 = InGroup2;
            IsBindGroup2Dirty = true;
        }
        if (InGroup3 != CurrentBindGroup3) {
            CurrentBindGroup3 = InGroup3;
            IsBindGroup3Dirty = true;;
        }
    }

    void MtlStateCache::ApplyDrawState(MTL::RenderCommandEncoder* RenderCommandEncoder)
    {
        check(RenderCommandEncoder);
        if(!CurrentViewPort.IsSet())
        {
            auto ColorAttachments = RenderPassDesc->colorAttachments();
            MTL::Texture* Rt = ColorAttachments->object(0)->texture();
            if(Rt)
            {
                MTL::Viewport Viewport{
                    0, 0,
                    (double)Rt->width(), (double)Rt->height(),
                    0, 1.0
                };

                MTL::ScissorRect ScissorRect{ 0, 0, Rt->width(), Rt->height() };

                SetViewPort(MoveTemp(Viewport), MoveTemp(ScissorRect));
            }
        }
        
        if(IsRenderPipelineDirty)
        {
            check(CurrentRenderPipelineState);
            RenderCommandEncoder->setRenderPipelineState(CurrentRenderPipelineState->GetResource());
            IsRenderPipelineDirty = false;
        }
        
        if(IsViewportDirty)
        {
			if (CurrentViewPort)
			{
                RenderCommandEncoder->setViewport(*CurrentViewPort);
                RenderCommandEncoder->setScissorRect(*CurrentScissorRect);
                IsViewportDirty = false;
			}
        }
        
        if(CurrentBindGroup0 && IsBindGroup0Dirty) 
		{ 
			CurrentBindGroup0->Apply(RenderCommandEncoder);
            IsBindGroup0Dirty = false;
		}
        if(CurrentBindGroup1 && IsBindGroup1Dirty)
		{ 
			CurrentBindGroup1->Apply(RenderCommandEncoder);
            IsBindGroup1Dirty = false;
		}
		if(CurrentBindGroup2 && IsBindGroup2Dirty)
		{ 
			CurrentBindGroup2->Apply(RenderCommandEncoder);
            IsBindGroup2Dirty = false;
		}
        if(CurrentBindGroup3 && IsBindGroup3Dirty)
		{ 
			CurrentBindGroup3->Apply(RenderCommandEncoder);
            IsBindGroup3Dirty = false;
		}
    }

                                       
   void MtlRenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
    {
        StateCache.ApplyDrawState(CmdEncoder.get());
        CmdEncoder->drawPrimitives(StateCache.GetPrimitiveType(), StartVertexLocation, VertexCount, InstanceCount, StartInstanceLocation);
    }
    
    void MtlRenderPassRecorder::SetRenderPipelineState(GpuPipelineState* InPipelineState)
    {
        StateCache.SetPipeline(static_cast<MetalRenderPipelineState*>(InPipelineState));
    }
    
    void MtlRenderPassRecorder::SetVertexBuffer(GpuBuffer* InVertexBuffer)
    {
        StateCache.SetVertexBuffer(static_cast<MetalBuffer*>(InVertexBuffer));
    }

    void MtlRenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
    {
        MTL::Viewport Viewport {
            (double)InViewPortDesc.TopLeftX, (double)InViewPortDesc.TopLeftY,
            (double)InViewPortDesc.Width, (double)InViewPortDesc.Height,
            (double)InViewPortDesc.ZMin, (double)InViewPortDesc.ZMax
        };

        MTL::ScissorRect ScissorRect { 0, 0, (uint32)InViewPortDesc.Width, (uint32)InViewPortDesc.Height };
        StateCache.SetViewPort(MoveTemp(Viewport), MoveTemp(ScissorRect));
    }

    void MtlRenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
    {
        StateCache.SetBindGroups(
            static_cast<MetalBindGroup *>(BindGroup0),
            static_cast<MetalBindGroup *>(BindGroup1),
            static_cast<MetalBindGroup *>(BindGroup2),
            static_cast<MetalBindGroup *>(BindGroup3)
        );
    }

    GpuRenderPassRecorder* MtlCmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        MTLRenderPassDescriptorPtr RenderPassDesc = NS::RetainPtr((MTL::RenderPassDescriptor*)MapRenderPassDesc(PassDesc));
        MTL::RenderCommandEncoder* Encoder= CmdBuffer->renderCommandEncoder(RenderPassDesc.get());
        Encoder->setLabel(FStringToNSString(PassName));
        
        auto PassRecorder = MakeUnique<MtlRenderPassRecorder>(NS::RetainPtr(Encoder), MoveTemp(RenderPassDesc));
        RenderPassRecorders.Add(MoveTemp(PassRecorder));
        return RenderPassRecorders.Last().Get();
    }

    void MtlCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
    {
        MtlRenderPassRecorder* PassRecorder = static_cast<MtlRenderPassRecorder*>(InRenderPassRecorder);
        PassRecorder->GetEncoder()->endEncoding();
    }

    void MtlCmdRecorder::BeginCaptureEvent(const FString& EventName)
    {
        
    }

    void MtlCmdRecorder::EndCaptureEvent()
    {
        
    }

    void MtlCmdRecorder::Barrier(GpuTrackedResource* InResource, GpuResourceState NewState)
    {
        InResource->State = NewState;
    }

    void MtlCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture)
    {
        MetalBuffer* SrcMtlBuffer = static_cast<MetalBuffer*>(InBuffer);
        MetalTexture* DstMtlTexture = static_cast<MetalTexture*>(InTexture);
        MTL::BlitCommandEncoder* BlitCommandEncoder = CmdBuffer->blitCommandEncoder();
        MTL::Origin Origin{0, 0, 0};
        MTL::Size Size{ InTexture->GetWidth(), InTexture->GetHeight(), 1};
        const uint32 BytesPerTexel = GetTextureFormatByteSize(InTexture->GetFormat());
        const uint32 BytesPerRow = InTexture->GetWidth() * BytesPerTexel;
        const uint32 BytesImage = InTexture->GetWidth() * InTexture->GetHeight() * BytesPerTexel;
        BlitCommandEncoder->copyFromBuffer(SrcMtlBuffer->GetResource(), 0, BytesPerRow, BytesImage, Size, DstMtlTexture->GetResource(), 0, 0, Origin);
        BlitCommandEncoder->endEncoding();
    }

    void MtlCmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
    {
        MTL::BlitCommandEncoder* BlitCommandEncoder = CmdBuffer->blitCommandEncoder();
        MTL::Origin Origin{0, 0, 0};
        MTL::Size Size{InTexture->GetWidth(), InTexture->GetHeight(), 1};
        MetalTexture* SrcMtlTexture = static_cast<MetalTexture*>(InTexture);
        MetalBuffer* DstMtlBuffer = static_cast<MetalBuffer*>(InBuffer);
        const uint32 BytesPerTexel = GetTextureFormatByteSize(InTexture->GetFormat());
        const uint32 BytesImage = InTexture->GetWidth() * InTexture->GetHeight() * BytesPerTexel;
        const uint32 RowPitch = InTexture->GetWidth() * BytesPerTexel;
        BlitCommandEncoder->copyFromTexture(SrcMtlTexture->GetResource(), 0, 0, Origin, Size, DstMtlBuffer->GetResource(), 0, RowPitch, BytesImage);
        BlitCommandEncoder->endEncoding();
    }
}
