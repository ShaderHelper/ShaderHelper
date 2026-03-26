#include "CommonHeader.h"
#include "MetalCommandRecorder.h"
#include "MetalMap.h"
#include "MetalGpuRhiBackend.h"
#include "MetalShader.h"

namespace FW
{
    MtlRenderStateCache::MtlRenderStateCache(MTLRenderPassDescriptorPtr InRenderPassDesc)
        : IsRenderPipelineDirty(false)
        , IsViewportDirty(false)
		, IsScissorRectDirty(false)
        , IsVertexBufferDirty(false)
        , IsBindGroup0Dirty(false)
        , IsBindGroup1Dirty(false)
        , IsBindGroup2Dirty(false)
        , IsBindGroup3Dirty(false)
        , CurrentRenderPipelineState(nullptr)
        , CurrentIndexBuffer(nullptr)
		, CurrentIndexFormat(GpuFormat::R16_UINT)
        , CurrentIndexOffset(0)
        , CurrentBindGroup0(nullptr)
        , CurrentBindGroup1(nullptr)
        , CurrentBindGroup2(nullptr)
        , CurrentBindGroup3(nullptr)
        , RenderPassDesc(MoveTemp(InRenderPassDesc))
    {
	}

    MtlComputeStateCache::MtlComputeStateCache()
        : IsComputePipelineDirty(false)
        , IsBindGroup0Dirty(false)
        , IsBindGroup1Dirty(false)
        , IsBindGroup2Dirty(false)
        , IsBindGroup3Dirty(false)
        , CurrentComputePipelineState(nullptr)
        , CurrentBindGroup0(nullptr)
        , CurrentBindGroup1(nullptr)
        , CurrentBindGroup2(nullptr)
        , CurrentBindGroup3(nullptr)
    {}

    void MtlRenderStateCache::SetPipeline(MetalRenderPipelineState* InPipelineState)
    {
        if (InPipelineState != CurrentRenderPipelineState)
        {
            CurrentRenderPipelineState = InPipelineState;
            IsRenderPipelineDirty = true;
        }
    }

    void MtlRenderStateCache::SetVertexBuffer(uint32 Slot, MetalBuffer* InBuffer, uint32 Offset)
    {
        if (CurrentVertexBuffers[Slot].Buffer != InBuffer || CurrentVertexBuffers[Slot].Offset != Offset)
        {
            CurrentVertexBuffers[Slot] = { InBuffer, Offset };
            IsVertexBufferDirty = true;
        }
    }

    void MtlRenderStateCache::SetIndexBuffer(MetalBuffer* InBuffer, GpuFormat InIndexFormat, uint32 Offset)
    {
        if (CurrentIndexBuffer != InBuffer || CurrentIndexFormat != InIndexFormat || CurrentIndexOffset != Offset)
        {
            CurrentIndexBuffer = InBuffer;
            CurrentIndexFormat = InIndexFormat;
            CurrentIndexOffset = Offset;
        }
    }

    void MtlRenderStateCache::SetViewPort(MTL::Viewport InViewPort)
    {
        if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort , &InViewPort, sizeof(MTL::Viewport)))
        {
            CurrentViewPort = MoveTemp(InViewPort);
            IsViewportDirty = true;
        }
    }

	void MtlRenderStateCache::SetScissorRect(MTL::ScissorRect InSissorRect)
	{
		if (!CurrentScissorRect || FMemory::Memcmp(&*CurrentScissorRect, &InSissorRect, sizeof(MTL::ScissorRect)))
		{
			CurrentScissorRect = MoveTemp(InSissorRect);
			IsScissorRectDirty = true;
		}
	}

	void MtlRenderStateCache::SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3)
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

    void MtlRenderStateCache::ApplyDrawState(MTL::RenderCommandEncoder* RenderCommandEncoder)
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

                SetViewPort(MoveTemp(Viewport));
            }
        }
		if (!CurrentScissorRect.IsSet() && CurrentViewPort)
		{
			MTL::ScissorRect ScissorRect{ 0, 0, (uint32)(*CurrentViewPort).width, (uint32)(*CurrentViewPort).height };
			SetScissorRect(MoveTemp(ScissorRect));
		}
        
        if(IsRenderPipelineDirty)
        {
            check(CurrentRenderPipelineState);
            RenderCommandEncoder->setRenderPipelineState(CurrentRenderPipelineState->GetResource());
            RenderCommandEncoder->setFrontFacingWinding(MTL::WindingClockwise);
            RenderCommandEncoder->setCullMode(static_cast<MTL::CullMode>(MapRasterizerCullMode(CurrentRenderPipelineState->GetDesc().RasterizerState.CullMode)));
            IsRenderPipelineDirty = false;
        }
        
        if(IsViewportDirty && CurrentViewPort)
        {
			RenderCommandEncoder->setViewport(*CurrentViewPort);
			IsViewportDirty = false;
        }
		if(IsScissorRectDirty && CurrentScissorRect)
		{
			RenderCommandEncoder->setScissorRect(*CurrentScissorRect);
			IsScissorRectDirty = false;
		}

        if (IsVertexBufferDirty && CurrentRenderPipelineState)
        {
            for (int32 BufferSlot = 0; BufferSlot < CurrentRenderPipelineState->GetDesc().VertexLayout.Num(); ++BufferSlot)
            {
                const VertexBufferBinding& Binding = CurrentVertexBuffers[BufferSlot];
                if (!Binding.Buffer)
                {
                    continue;
                }
                RenderCommandEncoder->setVertexBuffer(Binding.Buffer->GetResource(), Binding.Offset, BufferSlot + GpuResourceLimit::MaxBindableBingGroupNum);
            }
            IsVertexBufferDirty = false;
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

    void MtlComputeStateCache::ApplyComputeState(MTL::ComputeCommandEncoder* ComputeCommandEncoder)
    {
        check(ComputeCommandEncoder);
        if(IsComputePipelineDirty)
        {
            check(CurrentComputePipelineState);
            ComputeCommandEncoder->setComputePipelineState(CurrentComputePipelineState->GetResource());
            IsComputePipelineDirty = false;
        }

        if(CurrentBindGroup0 && IsBindGroup0Dirty) 
		{ 
			CurrentBindGroup0->Apply(ComputeCommandEncoder);
            IsBindGroup0Dirty = false;
		}
        if(CurrentBindGroup1 && IsBindGroup1Dirty)
		{ 
			CurrentBindGroup1->Apply(ComputeCommandEncoder);
            IsBindGroup1Dirty = false;
		}
		if(CurrentBindGroup2 && IsBindGroup2Dirty)
		{ 
			CurrentBindGroup2->Apply(ComputeCommandEncoder);
            IsBindGroup2Dirty = false;
		}
        if(CurrentBindGroup3 && IsBindGroup3Dirty)
		{ 
			CurrentBindGroup3->Apply(ComputeCommandEncoder);
            IsBindGroup3Dirty = false;
		}
    }

    void MtlComputeStateCache::SetPipeline(MetalComputePipelineState* InPipelineState)
    {
        if (InPipelineState != CurrentComputePipelineState)
        {
            CurrentComputePipelineState = InPipelineState;
            IsComputePipelineDirty = true;
        }
    }

    void MtlComputeStateCache::SetBindGroups(MetalBindGroup* InGroup0, MetalBindGroup* InGroup1, MetalBindGroup* InGroup2, MetalBindGroup* InGroup3)
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
                                       
   void MtlRenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
    {
        StateCache.ApplyDrawState(CmdEncoder.get());
		CmdEncoder->drawPrimitives(StateCache.GetPrimitiveType(), StartVertexLocation, VertexCount, InstanceCount, StartInstanceLocation);
    }

    void MtlRenderPassRecorder::DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount)
    {
        StateCache.ApplyDrawState(CmdEncoder.get());
        CmdEncoder->drawIndexedPrimitives(StateCache.GetPrimitiveType(), IndexCount, static_cast<MTL::IndexType>(MapIndexFormat(StateCache.GetIndexFormat())), StateCache.GetIndexBuffer()->GetResource(), StateCache.GetIndexOffset(), InstanceCount, BaseVertexLocation, StartInstanceLocation);
    }
    
    void MtlRenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
    {
        StateCache.SetPipeline(static_cast<MetalRenderPipelineState*>(InPipelineState));
    }
    
    void MtlRenderPassRecorder::SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset)
    {
        StateCache.SetVertexBuffer(Slot, static_cast<MetalBuffer*>(InVertexBuffer), Offset);
    }

    void MtlRenderPassRecorder::SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset)
    {
        StateCache.SetIndexBuffer(static_cast<MetalBuffer*>(InIndexBuffer), IndexFormat, Offset);
    }

    void MtlRenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
    {
        MTL::Viewport Viewport {
            (double)InViewPortDesc.TopLeftX, (double)InViewPortDesc.TopLeftY,
            (double)InViewPortDesc.Width, (double)InViewPortDesc.Height,
            (double)InViewPortDesc.ZMin, (double)InViewPortDesc.ZMax
        };

        StateCache.SetViewPort(MoveTemp(Viewport));
    }

	void MtlRenderPassRecorder::SetScissorRect(const GpuScissorRectDesc& InScissorRectDes)
	{
		MTL::ScissorRect ScissorRect{ InScissorRectDes.Left, InScissorRectDes.Top, InScissorRectDes.Right, InScissorRectDes.Bottom };
		StateCache.SetScissorRect(MoveTemp(ScissorRect));
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

    void MtlComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
    {
        StateCache.ApplyComputeState(CmdEncoder.get());
        MetalComputePipelineState* Pipeline = StateCache.GetPipeline();
        MetalShader* Cs = static_cast<MetalShader*>(Pipeline->GetDesc().Cs);
        CmdEncoder->dispatchThreadgroups(MTL::Size{ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ},
        MTL::Size{Cs->ThreadGroupSize.X, Cs->ThreadGroupSize.Y, Cs->ThreadGroupSize.Z});
    }

    void MtlComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
    {
        StateCache.SetPipeline(static_cast<MetalComputePipelineState*>(InPipelineState));
    }

    void MtlComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
    {
        StateCache.SetBindGroups(
            static_cast<MetalBindGroup *>(BindGroup0),
            static_cast<MetalBindGroup *>(BindGroup1),
            static_cast<MetalBindGroup *>(BindGroup2),
            static_cast<MetalBindGroup *>(BindGroup3)
        );
    }

    GpuComputePassRecorder* MtlCmdRecorder::BeginComputePass(const FString& PassName)
    {
        MTL::ComputeCommandEncoder* Encoder = CmdBuffer->computeCommandEncoder();
        Encoder->setLabel(FStringToNSString(PassName));
        auto PassRecorder = MakeUnique<MtlComputePassRecorder>(NS::RetainPtr(Encoder));
        ComputePassRecorders.Add(MoveTemp(PassRecorder));
        return ComputePassRecorders.Last().Get();
    }

    void MtlCmdRecorder::EndComputePass(GpuComputePassRecorder* InComputePassRecorder)
    {
        MtlComputePassRecorder* PassRecorder = static_cast<MtlComputePassRecorder*>(InComputePassRecorder);
        PassRecorder->GetEncoder()->endEncoding();
    }

    GpuRenderPassRecorder* MtlCmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        MTLRenderPassDescriptorPtr RenderPassDesc = NS::RetainPtr((MTL::RenderPassDescriptor*)MapRenderPassDesc(PassDesc));
        MTL::RenderCommandEncoder* RenderCommandEncoder = CmdBuffer->renderCommandEncoder(RenderPassDesc.get());
        RenderCommandEncoder->setLabel(FStringToNSString(PassName));
        auto PassRecorder = MakeUnique<MtlRenderPassRecorder>(NS::RetainPtr(RenderCommandEncoder), MoveTemp(RenderPassDesc));

        if (PassDesc.TimestampWrites && !GSupportStageBoundaryCounter)
        {
            MetalQuerySet* MtlQuerySet = static_cast<MetalQuerySet*>(PassDesc.TimestampWrites->QuerySet);
            PassRecorder->TimestampSampleBuffer = MtlQuerySet->GetSampleBuffer();
            PassRecorder->EndOfPassSampleIndex = PassDesc.TimestampWrites->EndOfPassWriteIndex;
            RenderCommandEncoder->sampleCountersInBuffer(MtlQuerySet->GetSampleBuffer(), PassDesc.TimestampWrites->BeginningOfPassWriteIndex, true);
        }

        RenderPassRecorders.Add(MoveTemp(PassRecorder));
        return RenderPassRecorders.Last().Get();
    }

    void MtlCmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
    {
        MtlRenderPassRecorder* PassRecorder = static_cast<MtlRenderPassRecorder*>(InRenderPassRecorder);
        if (PassRecorder->TimestampSampleBuffer)
        {
            PassRecorder->GetEncoder()->sampleCountersInBuffer(PassRecorder->TimestampSampleBuffer, PassRecorder->EndOfPassSampleIndex, true);
        }
		PassRecorder->GetEncoder()->endEncoding();
    }

    void MtlCmdRecorder::BeginCaptureEvent(const FString& EventName)
    {
        
    }

    void MtlCmdRecorder::EndCaptureEvent()
    {
        
    }

    void MtlCmdRecorder::Barriers(const TArray<GpuBarrierInfo>& BarrierInfos)
    {
        for (const GpuBarrierInfo& Info : BarrierInfos)
        {
            if (Info.Resource->GetType() == GpuResourceType::Texture)
            {
                GpuTexture* Tex = static_cast<GpuTexture*>(Info.Resource);
                Tex->SetAllSubResourceStates(Info.NewState);
            }
            else if (Info.Resource->GetType() == GpuResourceType::TextureView)
            {
                GpuTextureView* View = static_cast<GpuTextureView*>(Info.Resource);
                GpuTexture* Tex = View->GetTexture();
                uint32 ArrayLayers = Tex->GetArrayLayerCount();
                for (uint32 i = 0; i < View->GetMipLevelCount(); i++)
                {
                    uint32 Mip = View->GetBaseMipLevel() + i;
                    for (uint32 Layer = 0; Layer < ArrayLayers; Layer++)
                    {
                        Tex->SetSubResourceState(Mip, Layer, Info.NewState);
                    }
                }
            }
            else
            {
                GpuBuffer* Buf = static_cast<GpuBuffer*>(Info.Resource);
                Buf->State = Info.NewState;
            }
        }
    }

    void MtlCmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer, uint32 MipLevel)
    {
        MetalBuffer* SrcMtlBuffer = static_cast<MetalBuffer*>(InBuffer);
        MetalTexture* DstMtlTexture = static_cast<MetalTexture*>(InTexture);
        MTL::BlitCommandEncoder* BlitCommandEncoder = CmdBuffer->blitCommandEncoder();
        MTL::Origin Origin{0, 0, 0};
        const uint32 Depth = InTexture->GetDepth();
        MTL::Size Size{ InTexture->GetWidth(), InTexture->GetHeight(), Depth};
        const uint32 BytesPerTexel = GetFormatByteSize(InTexture->GetFormat());
        const uint32 BytesPerRow = InTexture->GetWidth() * BytesPerTexel;
        const uint32 BytesImage = InTexture->GetWidth() * InTexture->GetHeight() * BytesPerTexel;
        BlitCommandEncoder->copyFromBuffer(SrcMtlBuffer->GetResource(), 0, BytesPerRow, BytesImage, Size, DstMtlTexture->GetResource(), ArrayLayer, MipLevel, Origin);
        BlitCommandEncoder->endEncoding();
    }

    void MtlCmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
    {
        MTL::BlitCommandEncoder* BlitCommandEncoder = CmdBuffer->blitCommandEncoder();
        MTL::Origin Origin{0, 0, 0};
        MTL::Size Size{InTexture->GetWidth(), InTexture->GetHeight(), 1};
        MetalTexture* SrcMtlTexture = static_cast<MetalTexture*>(InTexture);
        MetalBuffer* DstMtlBuffer = static_cast<MetalBuffer*>(InBuffer);
        const uint32 BytesPerTexel = GetFormatByteSize(InTexture->GetFormat());
        const uint32 BytesImage = InTexture->GetWidth() * InTexture->GetHeight() * BytesPerTexel;
        const uint32 RowPitch = InTexture->GetWidth() * BytesPerTexel;
        BlitCommandEncoder->copyFromTexture(SrcMtlTexture->GetResource(), 0, 0, Origin, Size, DstMtlBuffer->GetResource(), 0, RowPitch, BytesImage);
        BlitCommandEncoder->endEncoding();
    }

    void MtlCmdRecorder::CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size)
    {
		MetalBuffer* SrcMtlBuffer = static_cast<MetalBuffer*>(SrcBuffer);
		MetalBuffer* DstMtlBuffer = static_cast<MetalBuffer*>(DestBuffer);
		MTL::BlitCommandEncoder* BlitCommandEncoder = CmdBuffer->blitCommandEncoder();
		BlitCommandEncoder->copyFromBuffer(SrcMtlBuffer->GetResource(), SrcOffset, DstMtlBuffer->GetResource(), DestOffset, Size);
		BlitCommandEncoder->endEncoding();
    }
}
