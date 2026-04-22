#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"
#include "MetalTexture.h"
#include "MetalGpuRhiBackend.h"
#include "MetalPipeline.h"

namespace FW
{
    static int32 GetMetalArgumentIndex(BindingSlot InSlot)
    {
        return InSlot.SlotNum + GetBindingShift(InSlot.Type);
    }

    static bool HasRenderStage(BindingShaderStage InStage)
    {
        return EnumHasAnyFlags(InStage, BindingShaderStage::Vertex | BindingShaderStage::Pixel);
    }

    static MTLRenderStages MapShaderVisibility(BindingShaderStage InStage)
    {
        MTLRenderStages Stages = 0;
        
        if(EnumHasAnyFlags(InStage, BindingShaderStage::Vertex))
        {
            Stages |= MTLRenderStageVertex;
        }
        
        if(EnumHasAnyFlags(InStage, BindingShaderStage::Pixel))
        {
            Stages |= MTLRenderStageFragment;
        }

        return Stages;
    }

    MetalBindGroupLayout::MetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
        : GpuBindGroupLayout(LayoutDesc)
    {
        for (const auto& [Slot, LayoutBindingEntry] : Desc.Layouts)
        {
            if (HasRenderStage(LayoutBindingEntry.Stage))
            {
                RenderStages.Add(Slot, MapShaderVisibility(LayoutBindingEntry.Stage));
            }

            if (LayoutBindingEntry.Type == BindingType::RWStructuredBuffer || LayoutBindingEntry.Type == BindingType::RWRawBuffer)
            {
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else
            {
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
        }
    }

    MetalBindGroup::MetalBindGroup(const GpuBindGroupDesc& InDesc)
        : GpuBindGroup(InDesc)
    {
        GMtlDeferredReleaseManager.AddResource(this);
        
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(InDesc.Layout.GetReference());
        auto Layouts = BindGroupLayout->GetDesc().Layouts;

        for (const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
        {
            if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource.GetReference());
                BindGroupResources.Add(Slot, Buffer->GetResource());
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource.GetReference());
                BindGroupResources.Add(Slot, Tex->GetMtlDefaultView()->GetResource());
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::TextureView)
            {
                MetalTextureView* TexView = static_cast<MetalTextureView*>(ResourceBindingEntry.Resource.GetReference());
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::CombinedTextureSampler)
            {
                GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(ResourceBindingEntry.Resource.GetReference());
                MetalTextureView* TexView;
                if (Combined->GetTexture()->GetType() == GpuResourceType::Texture)
                    TexView = static_cast<MetalTexture*>(Combined->GetTexture())->GetMtlDefaultView();
                else
                    TexView = static_cast<MetalTextureView*>(Combined->GetTexture());
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
        }
    }

    void MetalBindGroup::EncodeResources(const MetalFunctionArgEncoder& StageEnc, BindingShaderStage Stage) const
    {
        MTL::ArgumentEncoder* ArgEncoder = StageEnc.Encoder.get();
        MetalBindGroupLayout* BGL = static_cast<MetalBindGroupLayout*>(GetLayout());
        const auto& LayoutDescs = BGL->GetDesc().Layouts;
        // Only write slots whose argument index exists in the compiled function's
        // argument-buffer struct.
        for (const auto& [Slot, ResourceBindingEntry] : GetDesc().Resources)
        {
            if (!EnumHasAnyFlags(LayoutDescs[Slot].Stage, Stage)) continue;

            const int32 ArgIdx = GetMetalArgumentIndex(Slot);

            if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::CombinedTextureSampler)
            {
                // CombinedTextureSampler expands to two reflected indices: ArgIdx*2 (texture)
                // and ArgIdx*2+1 (sampler).
                GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(ResourceBindingEntry.Resource.GetReference());
                MetalTextureView* TexView;
                if (Combined->GetTexture()->GetType() == GpuResourceType::Texture)
                    TexView = static_cast<MetalTexture*>(Combined->GetTexture())->GetMtlDefaultView();
                else
                    TexView = static_cast<MetalTextureView*>(Combined->GetTexture());
                MetalSampler* Sampler = static_cast<MetalSampler*>(Combined->GetSampler());
                const int32 TexIndex     = ArgIdx * 2;
                const int32 SamplerIndex = ArgIdx * 2 + 1;
                if (StageEnc.ValidArgIndices.Contains(TexIndex))
                    ArgEncoder->setTexture(TexView->GetResource(), TexIndex);
                if (StageEnc.ValidArgIndices.Contains(SamplerIndex))
                    ArgEncoder->setSamplerState(Sampler->GetResource(), SamplerIndex);
                continue;
            }

            if (!StageEnc.ValidArgIndices.Contains(ArgIdx)) continue;

            if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource.GetReference());
                ArgEncoder->setBuffer(Buffer->GetResource(), 0, ArgIdx);
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource.GetReference());
                ArgEncoder->setTexture(Tex->GetMtlDefaultView()->GetResource(), ArgIdx);
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::TextureView)
            {
                MetalTextureView* TexView = static_cast<MetalTextureView*>(ResourceBindingEntry.Resource.GetReference());
                ArgEncoder->setTexture(TexView->GetResource(), ArgIdx);
            }
            else if (ResourceBindingEntry.Resource->GetType() == GpuResourceType::Sampler)
            {
                MetalSampler* Sampler = static_cast<MetalSampler*>(ResourceBindingEntry.Resource.GetReference());
                ArgEncoder->setSamplerState(Sampler->GetResource(), ArgIdx);
            }
        }
    }

    MetalBuffer* MetalBindGroup::GetOrCreateArgBuffer(const MetalFunctionArgEncoder& StageEnc)
    {
        MTL::ArgumentEncoder* ArgEncoder = StageEnc.Encoder.get();
        TRefCountPtr<MetalBuffer>* Cached = ArgBufferCache.Find(ArgEncoder);
        if (!Cached)
        {
            TRefCountPtr<MetalBuffer> NewBuffer = CreateMetalBuffer({(uint32)ArgEncoder->encodedLength(), GpuBufferUsage::Upload});
            ArgEncoder->setArgumentBuffer(NewBuffer->GetResource(), 0);
            ArgBufferCache.Add(ArgEncoder, MoveTemp(NewBuffer));
            return ArgBufferCache[ArgEncoder].GetReference();
        }
        ArgEncoder->setArgumentBuffer((*Cached)->GetResource(), 0);
        return Cached->GetReference();
    }

    void MetalBindGroup::Apply(MTL::RenderCommandEncoder* Encoder, MetalRenderPipelineState* Pipeline)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        int32 SetNumber = BindGroupLayout->GetGroupNumber();
        auto Layouts = BindGroupLayout->GetDesc().Layouts;

        for (auto [Slot, MtlResource] : BindGroupResources)
        {
            if (Layouts[Slot].Stage != BindingShaderStage::Compute)
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot), BindGroupLayout->GetRenderStages(Slot));
            }
        }

        if (const MetalFunctionArgEncoder* VsEnc = Pipeline->GetVsArgumentEncoder(SetNumber))
        {
            MetalBuffer* ArgBuffer = GetOrCreateArgBuffer(*VsEnc);
            EncodeResources(*VsEnc, BindingShaderStage::Vertex);
            Encoder->setVertexBuffer(ArgBuffer->GetResource(), 0, SetNumber);
        }

        if (const MetalFunctionArgEncoder* FsEnc = Pipeline->GetFsArgumentEncoder(SetNumber))
        {
            MetalBuffer* ArgBuffer = GetOrCreateArgBuffer(*FsEnc);
            EncodeResources(*FsEnc, BindingShaderStage::Pixel);
            Encoder->setFragmentBuffer(ArgBuffer->GetResource(), 0, SetNumber);
        }
    }

    void MetalBindGroup::Apply(MTL::ComputeCommandEncoder* Encoder, MetalComputePipelineState* Pipeline)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        int32 SetNumber = BindGroupLayout->GetGroupNumber();
        auto Layouts = BindGroupLayout->GetDesc().Layouts;

        for (auto [Slot, MtlResource] : BindGroupResources)
        {
            if (EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot));
            }
        }

        if (const MetalFunctionArgEncoder* CsEnc = Pipeline->GetCsArgumentEncoder(SetNumber))
        {
            MetalBuffer* ArgBuffer = GetOrCreateArgBuffer(*CsEnc);
            EncodeResources(*CsEnc, BindingShaderStage::Compute);
            Encoder->setBuffer(ArgBuffer->GetResource(), 0, SetNumber);
        }
    }

    TRefCountPtr<MetalBindGroupLayout> CreateMetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
    {
        return new MetalBindGroupLayout(LayoutDesc);
    }

    TRefCountPtr<MetalBindGroup> CreateMetalBindGroup(const GpuBindGroupDesc& Desc)
    {
        return new MetalBindGroup(Desc);
    }
}


