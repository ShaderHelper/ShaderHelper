#include "CommonHeader.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"
#include "GpuApi/GpuApiValidation.h"

namespace FW
{
    MetalRenderPipelineState::MetalRenderPipelineState(GpuRenderPipelineStateDesc InDesc, MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType, MTLDepthStencilStatePtr InDepthStencilState)
    : GpuRenderPipelineState(MoveTemp(InDesc))
    , PipelineState(MoveTemp(InPipelineState))
    , PrimitiveType(InPrimitiveType)
    , DepthStencilState(MoveTemp(InDepthStencilState))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

    MetalComputePipelineState::MetalComputePipelineState(GpuComputePipelineStateDesc InDesc, MTLComputePipelineStatePtr InPipelineState)
    : GpuComputePipelineState(MoveTemp(InDesc))
    , PipelineState(MoveTemp(InPipelineState))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

	TRefCountPtr<MetalRenderPipelineState> CreateMetalRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
    {
        MetalShader* Vs = static_cast<MetalShader*>(InPipelineStateDesc.Vs);
        MetalShader* Ps = static_cast<MetalShader*>(InPipelineStateDesc.Ps);

		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Vs);
			if (Ps)
			{
				CheckShaderLayoutBinding(InPipelineStateDesc, Ps);
			}
		}
        
        MTLRenderPipelineDescriptorPtr PipelineDesc = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
        PipelineDesc->setVertexFunction(Vs->GetCompilationResult());
		if (Ps) {
			PipelineDesc->setFragmentFunction(Ps->GetCompilationResult());
		}

        if (InPipelineStateDesc.VertexLayout.Num() > 0)
        {
            MTL::VertexDescriptor* VertexDesc = MTL::VertexDescriptor::vertexDescriptor();
            for (int32 BufferSlot = 0; BufferSlot < InPipelineStateDesc.VertexLayout.Num(); ++BufferSlot)
            {
                const GpuVertexLayoutDesc& BufferLayout = InPipelineStateDesc.VertexLayout[BufferSlot];
                MTL::VertexBufferLayoutDescriptor* LayoutDesc = VertexDesc->layouts()->object(BufferSlot + GpuResourceLimit::MaxBindableBingGroupNum);
                LayoutDesc->setStride(BufferLayout.ByteStride);
                LayoutDesc->setStepFunction(static_cast<MTL::VertexStepFunction>(MapVertexStepMode(BufferLayout.StepMode)));
                LayoutDesc->setStepRate(BufferLayout.StepMode == GpuVertexStepMode::Instance ? 1 : 1);
                for (const GpuVertexAttributeDesc& Attribute : BufferLayout.Attributes)
                {
                    MTL::VertexAttributeDescriptor* AttributeDesc = VertexDesc->attributes()->object(Attribute.Location);
                    AttributeDesc->setFormat(static_cast<MTL::VertexFormat>(MapVertexFormat(Attribute.Format)));
                    AttributeDesc->setOffset(Attribute.ByteOffset);
                    AttributeDesc->setBufferIndex(BufferSlot + GpuResourceLimit::MaxBindableBingGroupNum);
                }
            }
            PipelineDesc->setVertexDescriptor(VertexDesc);
        }
        
        MTL::RenderPipelineColorAttachmentDescriptorArray* ColorAttachments = PipelineDesc->colorAttachments();
        for(uint32 i = 0; i < InPipelineStateDesc.Targets.Num(); i++)
        {
            const PipelineTargetDesc& Target = InPipelineStateDesc.Targets[i];

            MTL::RenderPipelineColorAttachmentDescriptor* ColorAttachment = ColorAttachments->object(i);
            ColorAttachment->setBlendingEnabled(Target.BlendEnable);
            ColorAttachment->setSourceRGBBlendFactor((MTL::BlendFactor)MapBlendFactor(Target.SrcFactor));
            ColorAttachment->setSourceAlphaBlendFactor((MTL::BlendFactor)MapBlendFactor(Target.SrcAlphaFactor));
            ColorAttachment->setDestinationRGBBlendFactor((MTL::BlendFactor)MapBlendFactor(Target.DestFactor));
            ColorAttachment->setDestinationAlphaBlendFactor((MTL::BlendFactor)MapBlendFactor(Target.DestAlphaFactor));
            ColorAttachment->setRgbBlendOperation((MTL::BlendOperation)MapBlendOp(Target.ColorOp));
            ColorAttachment->setAlphaBlendOperation((MTL::BlendOperation)MapBlendOp(Target.AlphaOp));
            ColorAttachment->setWriteMask(MapWriteMask(Target.Mask));
            ColorAttachment->setPixelFormat((MTL::PixelFormat)MapTextureFormat(Target.TargetFormat));
        }
        PipelineDesc->setRasterSampleCount(InPipelineStateDesc.SampleCount);

        if (InPipelineStateDesc.DepthStencilState)
        {
            PipelineDesc->setDepthAttachmentPixelFormat((MTL::PixelFormat)MapTextureFormat(InPipelineStateDesc.DepthStencilState->DepthFormat));
        }

        NS::Error* err = nullptr;
        MTLRenderPipelineStatePtr PipelineState = NS::TransferPtr(GDevice->newRenderPipelineState(PipelineDesc.get(), MTL::PipelineOptionNone ,nullptr ,&err));
        if (!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create render pipeline: %s"), *NSStringToFString(err->localizedDescription()));
        }

        MTLDepthStencilStatePtr DepthStencilState;
        if (InPipelineStateDesc.DepthStencilState)
        {
            MTL::DepthStencilDescriptor* DsDesc = MTL::DepthStencilDescriptor::alloc()->init();
            DsDesc->setDepthCompareFunction((MTL::CompareFunction)MapCompareFunction(InPipelineStateDesc.DepthStencilState->DepthCompare));
            DsDesc->setDepthWriteEnabled(InPipelineStateDesc.DepthStencilState->DepthWriteEnable);
            DepthStencilState = NS::TransferPtr(GDevice->newDepthStencilState(DsDesc));
            DsDesc->release();
        }
        
        return new MetalRenderPipelineState(InPipelineStateDesc, MoveTemp(PipelineState), MapPrimitiveType(InPipelineStateDesc.Primitive), MoveTemp(DepthStencilState));
    }

    TRefCountPtr<MetalComputePipelineState> CreateMetalComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
    {
        MetalShader* Cs = static_cast<MetalShader*>(InPipelineStateDesc.Cs);
		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Cs);
		}

        NS::Error* err = nullptr;
        MTLComputePipelineStatePtr PipelineState = NS::TransferPtr(GDevice->newComputePipelineState(Cs->GetCompilationResult(), &err));
        if(!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create compute pipeline: %s"), *NSStringToFString(err->localizedDescription()));
        }
        return new MetalComputePipelineState(InPipelineStateDesc, MoveTemp(PipelineState));
    }
}
