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
        , IsBindGroupDirty{}
        , CurrentRenderPipelineState(nullptr)
        , CurrentIndexBuffer(nullptr)
		, CurrentIndexFormat(GpuFormat::R16_UINT)
        , CurrentIndexOffset(0)
        , RenderPassDesc(MoveTemp(InRenderPassDesc))
    {
	}

    MtlComputeStateCache::MtlComputeStateCache()
        : IsComputePipelineDirty(false)
        , IsBindGroupDirty{}
        , CurrentComputePipelineState(nullptr)
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
        MTL::RenderPassColorAttachmentDescriptor* Attachment = RenderPassDesc->colorAttachments()->object(0);
        MTL::Texture* RenderTarget = Attachment->texture();
        
        const uint32 PassWidth = static_cast<uint32>(RenderTarget->width());
        const uint32 PassHeight = static_cast<uint32>(RenderTarget->height());
        const uint32 ClampedX = FMath::Min<uint32>(InSissorRect.x, PassWidth);
        const uint32 ClampedY = FMath::Min<uint32>(InSissorRect.y, PassHeight);
        InSissorRect.x = ClampedX;
        InSissorRect.y = ClampedY;
        InSissorRect.width = FMath::Min<uint32>(InSissorRect.width, PassWidth - ClampedX);
        InSissorRect.height = FMath::Min<uint32>(InSissorRect.height, PassHeight - ClampedY);

		if (!CurrentScissorRect || FMemory::Memcmp(&*CurrentScissorRect, &InSissorRect, sizeof(MTL::ScissorRect)))
		{
			CurrentScissorRect = MoveTemp(InSissorRect);
			IsScissorRectDirty = true;
		}
	}

	void MtlRenderStateCache::SetBindGroups(const TArray<MetalBindGroup*>& InGroups)
    {
        for (MetalBindGroup* Group : InGroups)
        {
            if (!Group) continue;
            int32 Slot = Group->GetLayout()->GetGroupNumber();
            if (Group != CurrentBindGroups[Slot]) {
                CurrentBindGroups[Slot] = Group;
                IsBindGroupDirty[Slot] = true;
            }
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
            const uint32 ScissorX = (uint32)(*CurrentViewPort).originX;
            const uint32 ScissorY = (uint32)(*CurrentViewPort).originY;
            const uint32 ScissorWidth = (uint32)((*CurrentViewPort).originX + (*CurrentViewPort).width) - ScissorX;
            const uint32 ScissorHeight = (uint32)((*CurrentViewPort).originY + (*CurrentViewPort).height) - ScissorY;
            MTL::ScissorRect ScissorRect{ ScissorX, ScissorY, ScissorWidth, ScissorHeight };
			SetScissorRect(MoveTemp(ScissorRect));
		}
        
        if(IsRenderPipelineDirty)
        {
            check(CurrentRenderPipelineState);
            RenderCommandEncoder->setRenderPipelineState(CurrentRenderPipelineState->GetResource());
            RenderCommandEncoder->setFrontFacingWinding(MTL::WindingClockwise);
            RenderCommandEncoder->setCullMode(static_cast<MTL::CullMode>(MapRasterizerCullMode(CurrentRenderPipelineState->GetDesc().RasterizerState.CullMode)));
            RenderCommandEncoder->setTriangleFillMode(static_cast<MTL::TriangleFillMode>(MapRasterizerFillMode(CurrentRenderPipelineState->GetDesc().RasterizerState.FillMode)));
            if (CurrentRenderPipelineState->GetDepthStencilState())
            {
                RenderCommandEncoder->setDepthStencilState(CurrentRenderPipelineState->GetDepthStencilState());
            }
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
        
        for (int32 i = 0; i < GpuResourceLimit::MaxBindableBingGroupNum; ++i)
        {
            if (CurrentBindGroups[i] && IsBindGroupDirty[i])
            {
                CurrentBindGroups[i]->Apply(RenderCommandEncoder);
                IsBindGroupDirty[i] = false;
            }
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

        for (int32 i = 0; i < GpuResourceLimit::MaxBindableBingGroupNum; ++i)
        {
            if (CurrentBindGroups[i] && IsBindGroupDirty[i])
            {
                CurrentBindGroups[i]->Apply(ComputeCommandEncoder);
                IsBindGroupDirty[i] = false;
            }
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

    void MtlComputeStateCache::SetBindGroups(const TArray<MetalBindGroup*>& InGroups)
    {
        for (MetalBindGroup* Group : InGroups)
        {
            if (!Group) continue;
            int32 Slot = Group->GetLayout()->GetGroupNumber();
            if (Group != CurrentBindGroups[Slot]) {
                CurrentBindGroups[Slot] = Group;
                IsBindGroupDirty[Slot] = true;
            }
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

    void MtlRenderPassRecorder::SetBindGroups(const TArray<GpuBindGroup*>& BindGroups)
    {
        TArray<MetalBindGroup*> MtlBindGroups;
        for (GpuBindGroup* Group : BindGroups)
        {
            MtlBindGroups.Add(static_cast<MetalBindGroup*>(Group));
        }
        StateCache.SetBindGroups(MtlBindGroups);
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

    void MtlComputePassRecorder::SetBindGroups(const TArray<GpuBindGroup*>& BindGroups)
    {
        TArray<MetalBindGroup*> MtlBindGroups;
        for (GpuBindGroup* Group : BindGroups)
        {
            MtlBindGroups.Add(static_cast<MetalBindGroup*>(Group));
        }
        StateCache.SetBindGroups(MtlBindGroups);
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
                SetLocalTextureAllSubResourceStates(Tex, Info.NewState);
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
                        SetLocalTextureSubResourceState(Tex, Mip, Layer, Info.NewState);
                    }
                }
            }
            else
            {
                GpuBuffer* Buf = static_cast<GpuBuffer*>(Info.Resource);
                SetLocalBufferState(Buf, Info.NewState);
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
