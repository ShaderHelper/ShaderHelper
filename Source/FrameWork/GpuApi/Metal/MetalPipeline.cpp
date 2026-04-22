#include "CommonHeader.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalMap.h"
#include "MetalDevice.h"
#include "MetalGpuRhiBackend.h"
#include "GpuApi/GpuApiValidation.h"

namespace FW
{
    MetalRenderPipelineState::MetalRenderPipelineState(GpuRenderPipelineStateDesc InDesc, MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType, MTLDepthStencilStatePtr InDepthStencilState,
        TMap<int32, MetalFunctionArgEncoder> InVsArgumentEncoders, TMap<int32, MetalFunctionArgEncoder> InFsArgumentEncoders)
    : GpuRenderPipelineState(MoveTemp(InDesc))
    , PipelineState(MoveTemp(InPipelineState))
    , PrimitiveType(InPrimitiveType)
    , DepthStencilState(MoveTemp(InDepthStencilState))
    , VsArgumentEncoders(MoveTemp(InVsArgumentEncoders))
    , FsArgumentEncoders(MoveTemp(InFsArgumentEncoders))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

    MetalComputePipelineState::MetalComputePipelineState(GpuComputePipelineStateDesc InDesc, MTLComputePipelineStatePtr InPipelineState,
        TMap<int32, MetalFunctionArgEncoder> InCsArgumentEncoders)
    : GpuComputePipelineState(MoveTemp(InDesc))
    , PipelineState(MoveTemp(InPipelineState))
    , CsArgumentEncoders(MoveTemp(InCsArgumentEncoders))
    {
        GMtlDeferredReleaseManager.AddResource(this);
    }

	// Create an argument encoder for the given function / buffer-slot and collect
    // the argument-buffer indices that the compiled function actually declares.
    static MetalFunctionArgEncoder BuildFunctionArgEncoder(MTL::Function* Function, int32 SetNumber)
    {
        MetalFunctionArgEncoder Result;
        MTL::AutoreleasedArgument Reflection = nullptr;
        MTL::ArgumentEncoder* Enc = Function->newArgumentEncoder((NS::UInteger)SetNumber, &Reflection);
        Result.Encoder = NS::TransferPtr(Enc);
        if (Reflection && Reflection->bufferDataType() == MTL::DataTypeStruct)
        {
            NS::Array* Members = Reflection->bufferStructType()->members();
            for (NS::UInteger i = 0; i < Members->count(); i++)
            {
                auto* Member = static_cast<MTL::StructMember*>(Members->object(i));
                Result.ValidArgIndices.Add((int32)Member->argumentIndex());
            }
        }
        return Result;
    }

    // Collect the buffer-slot indices declared by a stage from pipeline reflection bindings.
    static TSet<int32> CollectBufferSlots(NS::Array* Bindings)
    {
        TSet<int32> Result;
        if (!Bindings) return Result;
        for (NS::UInteger i = 0; i < Bindings->count(); i++)
        {
            auto* B = static_cast<MTL::Binding*>(Bindings->object(i));
            if (B->type() == MTL::BindingTypeBuffer)
                Result.Add((int32)B->index());
        }
        return Result;
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
        MTL::AutoreleasedRenderPipelineReflection PipelineReflection = nullptr;
        MTLRenderPipelineStatePtr PipelineState = NS::TransferPtr(GDevice->newRenderPipelineState(PipelineDesc.get(), MTL::PipelineOptionArgumentInfo, &PipelineReflection, &err));
        if (!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create render pipeline: %s"), *NSStringToFString(err->localizedDescription()));
        }

        // Which [[buffer(N)]] slots each stage actually declares — used to guard
        // newArgumentEncoder calls (it fatally asserts for absent indices).
        TSet<int32> VsBufferSlots = CollectBufferSlots(PipelineReflection->vertexBindings());
        TSet<int32> FsBufferSlots = CollectBufferSlots(PipelineReflection->fragmentBindings());

        MTLDepthStencilStatePtr DepthStencilState;
        if (InPipelineStateDesc.DepthStencilState)
        {
            MTL::DepthStencilDescriptor* DsDesc = MTL::DepthStencilDescriptor::alloc()->init();
            DsDesc->setDepthCompareFunction((MTL::CompareFunction)MapCompareFunction(InPipelineStateDesc.DepthStencilState->DepthCompare));
            DsDesc->setDepthWriteEnabled(InPipelineStateDesc.DepthStencilState->DepthWriteEnable);
            DepthStencilState = NS::TransferPtr(GDevice->newDepthStencilState(DsDesc));
            DsDesc->release();
        }

        // Build per-set argument encoders from the compiled functions.
        TMap<int32, MetalFunctionArgEncoder> VsEncoders, FsEncoders;
        for (GpuBindGroupLayout* Layout : InPipelineStateDesc.BindGroupLayouts)
        {
            if (!Layout) continue;
            int32 SetNumber = Layout->GetGroupNumber();
            if (VsBufferSlots.Contains(SetNumber))
            {
                MetalFunctionArgEncoder VsEnc = BuildFunctionArgEncoder(Vs->GetCompilationResult(), SetNumber);
                VsEncoders.Add(SetNumber, MoveTemp(VsEnc));
            }
            if (Ps && FsBufferSlots.Contains(SetNumber))
            {
                MetalFunctionArgEncoder FsEnc = BuildFunctionArgEncoder(Ps->GetCompilationResult(), SetNumber);
                FsEncoders.Add(SetNumber, MoveTemp(FsEnc));
            }
        }
        
        return new MetalRenderPipelineState(InPipelineStateDesc, MoveTemp(PipelineState), MapPrimitiveType(InPipelineStateDesc.Primitive), MoveTemp(DepthStencilState),
            MoveTemp(VsEncoders), MoveTemp(FsEncoders));
    }

    TRefCountPtr<MetalComputePipelineState> CreateMetalComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
    {
        MetalShader* Cs = static_cast<MetalShader*>(InPipelineStateDesc.Cs);
		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Cs);
		}

        NS::Error* err = nullptr;
        MTL::AutoreleasedComputePipelineReflection PipelineReflection = nullptr;
        MTLComputePipelineStatePtr PipelineState = NS::TransferPtr(GDevice->newComputePipelineState(Cs->GetCompilationResult(), MTL::PipelineOptionArgumentInfo, &PipelineReflection, &err));
        if(!PipelineState)
        {
            SH_LOG(LogMetal, Fatal, TEXT("Failed to create compute pipeline: %s"), *NSStringToFString(err->localizedDescription()));
        }

        TSet<int32> CsBufferSlots = CollectBufferSlots(PipelineReflection ? PipelineReflection->bindings() : nullptr);

        TMap<int32, MetalFunctionArgEncoder> CsEncoders;
        for (GpuBindGroupLayout* Layout : InPipelineStateDesc.BindGroupLayouts)
        {
            if (!Layout) continue;
            int32 SetNumber = Layout->GetGroupNumber();
            if (CsBufferSlots.Contains(SetNumber))
            {
                MetalFunctionArgEncoder CsEnc = BuildFunctionArgEncoder(Cs->GetCompilationResult(), SetNumber);
                CsEncoders.Add(SetNumber, MoveTemp(CsEnc));
            }
        }

        return new MetalComputePipelineState(InPipelineStateDesc, MoveTemp(PipelineState), MoveTemp(CsEncoders));
    }
}
