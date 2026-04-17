#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"
#include "MetalTexture.h"
#include "MetalGpuRhiBackend.h"

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
        TArray<MTLArgumentDescriptor*> VertexArgDescs;
        TArray<MTLArgumentDescriptor*> FragmentArgDescs;
        TArray<MTLArgumentDescriptor*> ComputeArgDescs;

        for(const auto& [Slot, LayoutBindingEntry] : Desc.Layouts)
        {
            if(HasRenderStage(LayoutBindingEntry.Stage))
            {
                RenderStages.Add(Slot, MapShaderVisibility(LayoutBindingEntry.Stage));
            }
            
            MTLArgumentDescriptor* ArgDesc = [MTLArgumentDescriptor argumentDescriptor];
            ArgDesc.index = GetMetalArgumentIndex(Slot);
            MTLArgumentDescriptor* CombinedSamplerArgDesc = nil;
            if(LayoutBindingEntry.Type == BindingType::RWStructuredBuffer || LayoutBindingEntry.Type == BindingType::RWRawBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::UniformBuffer || LayoutBindingEntry.Type == BindingType::StructuredBuffer || LayoutBindingEntry.Type == BindingType::RawBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::CombinedTextureSampler || LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler || LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler)
            {
                // SPIRV-Cross splits a combined sampler binding N into separate
                // texture/sampler arguments at N * 2 and N * 2 + 1.
                ArgDesc.index = GetMetalArgumentIndex(Slot) * 2;
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler ? MTLTextureTypeCube :
                    (LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler ? MTLTextureType3D : MTLTextureType2D);
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);

                CombinedSamplerArgDesc = [MTLArgumentDescriptor argumentDescriptor];
                CombinedSamplerArgDesc.index = GetMetalArgumentIndex(Slot) * 2 + 1;
                CombinedSamplerArgDesc.dataType = MTLDataTypeSampler;
                CombinedSamplerArgDesc.access = MTLArgumentAccessReadOnly;
            }
            else if(LayoutBindingEntry.Type == BindingType::Texture)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType2D;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::TextureCube)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureTypeCube;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::Texture3D)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType3D;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::RWTexture)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType2D;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::RWTexture3D)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType3D;
                ArgDesc.access = MTLArgumentAccessReadWrite;
                ResourceUsages.Add(Slot, MTLResourceUsageRead | MTLResourceUsageWrite);
            }
            else if(LayoutBindingEntry.Type == BindingType::Sampler)
            {
                ArgDesc.dataType = MTLDataTypeSampler;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else
            {
				AUX::Unreachable();
            }
            
            if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Vertex))
            {
                VertexArgDescs.Add(ArgDesc);
            }
            if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Pixel))
            {
                FragmentArgDescs.Add(ArgDesc);
            }
            if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Compute))
            {
                ComputeArgDescs.Add(ArgDesc);
            }
            
            if(CombinedSamplerArgDesc != nil)
            {
                if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Vertex))
                    VertexArgDescs.Add(CombinedSamplerArgDesc);
                if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Pixel))
                    FragmentArgDescs.Add(CombinedSamplerArgDesc);
                if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Compute))
                    ComputeArgDescs.Add(CombinedSamplerArgDesc);
            }
        }

        auto SortByIndex = [](MTLArgumentDescriptor* A, MTLArgumentDescriptor* B) {
            return A.index < B.index;
        };
        Algo::Sort(VertexArgDescs, SortByIndex);
        Algo::Sort(FragmentArgDescs, SortByIndex);
        Algo::Sort(ComputeArgDescs, SortByIndex);
        
        if(VertexArgDescs.Num() > 0)
        {
            VertexArgumentEncoder = NS::TransferPtr(GDevice->newArgumentEncoder(
                (NS::Array*)[NSArray arrayWithObjects:VertexArgDescs.GetData() count:VertexArgDescs.Num()]
            ));
        }
        
        if(FragmentArgDescs.Num() > 0)
        {
            FragmentArgumentEncoder = NS::TransferPtr(GDevice->newArgumentEncoder(
                (NS::Array*)[NSArray arrayWithObjects:FragmentArgDescs.GetData() count:FragmentArgDescs.Num()]
            ));
        }

        if(ComputeArgDescs.Num() > 0)
        {
            ComputeArgumentEncoder = NS::TransferPtr(GDevice->newArgumentEncoder(
                (NS::Array*)[NSArray arrayWithObjects:ComputeArgDescs.GetData() count:ComputeArgDescs.Num()]
            ));
        }

    }

    MetalBindGroup::MetalBindGroup(const GpuBindGroupDesc& InDesc)
        : GpuBindGroup(InDesc)
    {
        GMtlDeferredReleaseManager.AddResource(this);
        
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(InDesc.Layout.GetReference());
        MTL::ArgumentEncoder* VertexArgumentEncoder = BindGroupLayout->GetVertexArgumentEncoder();
        MTL::ArgumentEncoder* FragmentArgumentEncoder = BindGroupLayout->GetFragmentArgumentEncoder();
        MTL::ArgumentEncoder* ComputeArgumentEncoder = BindGroupLayout->GetComputeArgumentEncoder();

        if(VertexArgumentEncoder != nullptr)
        {
            VertexArgumentBuffer = CreateMetalBuffer({(uint32)VertexArgumentEncoder->encodedLength(), GpuBufferUsage::Upload});
            VertexArgumentEncoder->setArgumentBuffer(VertexArgumentBuffer->GetResource(), 0);
        }
        
        if(FragmentArgumentEncoder != nullptr)
        {
            FragmentArgumentBuffer = CreateMetalBuffer({(uint32)FragmentArgumentEncoder->encodedLength(), GpuBufferUsage::Upload});
            FragmentArgumentEncoder->setArgumentBuffer(FragmentArgumentBuffer->GetResource(), 0);
        }

        if(ComputeArgumentEncoder != nullptr)
        {
            ComputeArgumentBuffer = CreateMetalBuffer({(uint32)ComputeArgumentEncoder->encodedLength(), GpuBufferUsage::Upload});
            ComputeArgumentEncoder->setArgumentBuffer(ComputeArgumentBuffer->GetResource(), 0);
        }
        
        auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
        {
            if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource.GetReference());
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Vertex))
                {
                    VertexArgumentEncoder->setBuffer(Buffer->GetResource(), 0, GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Pixel))
                {
                    FragmentArgumentEncoder->setBuffer(Buffer->GetResource(), 0, GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
                {
                    ComputeArgumentEncoder->setBuffer(Buffer->GetResource(), 0, GetMetalArgumentIndex(Slot));
                }
                BindGroupResources.Add(Slot, Buffer->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource.GetReference());
                MetalTextureView* TexView = Tex->GetMtlDefaultView();
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Vertex))
                {
                    VertexArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Pixel))
                {
                    FragmentArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
                {
                    ComputeArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::TextureView)
            {
                MetalTextureView* TexView = static_cast<MetalTextureView*>(ResourceBindingEntry.Resource.GetReference());
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Vertex))
                {
                    VertexArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Pixel))
                {
                    FragmentArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
                {
                    ComputeArgumentEncoder->setTexture(TexView->GetResource(), GetMetalArgumentIndex(Slot));
                }
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Sampler)
            {
                MetalSampler* Sampler = static_cast<MetalSampler*>(ResourceBindingEntry.Resource.GetReference());
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Vertex))
                {
                    VertexArgumentEncoder->setSamplerState(Sampler->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Pixel))
                {
                    FragmentArgumentEncoder->setSamplerState(Sampler->GetResource(), GetMetalArgumentIndex(Slot));
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
                {
                    ComputeArgumentEncoder->setSamplerState(Sampler->GetResource(), GetMetalArgumentIndex(Slot));
                }
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::CombinedTextureSampler)
            {
                GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(ResourceBindingEntry.Resource.GetReference());
                MetalTextureView* TexView;
                if (Combined->GetTexture()->GetType() == GpuResourceType::Texture)
                    TexView = static_cast<MetalTexture*>(Combined->GetTexture())->GetMtlDefaultView();
                else
                    TexView = static_cast<MetalTextureView*>(Combined->GetTexture());
                MetalSampler* Sampler = static_cast<MetalSampler*>(Combined->GetSampler());
                int32 TexIndex = GetMetalArgumentIndex(Slot) * 2;
                int32 SamplerIndex = GetMetalArgumentIndex(Slot) * 2 + 1;
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Vertex))
                {
                    VertexArgumentEncoder->setTexture(TexView->GetResource(), TexIndex);
                    VertexArgumentEncoder->setSamplerState(Sampler->GetResource(), SamplerIndex);
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Pixel))
                {
                    FragmentArgumentEncoder->setTexture(TexView->GetResource(), TexIndex);
                    FragmentArgumentEncoder->setSamplerState(Sampler->GetResource(), SamplerIndex);
                }
                if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
                {
                    ComputeArgumentEncoder->setTexture(TexView->GetResource(), TexIndex);
                    ComputeArgumentEncoder->setSamplerState(Sampler->GetResource(), SamplerIndex);
                }
                BindGroupResources.Add(Slot, TexView->GetResource());
            }
            else
            {
				AUX::Unreachable();
            }
        }
    }

    void MetalBindGroup::Apply(MTL::RenderCommandEncoder* Encoder)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            if(Layouts[Slot].Stage != BindingShaderStage::Compute)
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot), BindGroupLayout->GetRenderStages(Slot));
            }
        }
        
        if(VertexArgumentBuffer)
        {
            Encoder->setVertexBuffer(VertexArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
        }
        
        if(FragmentArgumentBuffer)
        {
            Encoder->setFragmentBuffer(FragmentArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
        }
        
    }

    void MetalBindGroup::Apply(MTL::ComputeCommandEncoder* Encoder)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        auto Layouts = BindGroupLayout->GetDesc().Layouts;
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            if(EnumHasAnyFlags(Layouts[Slot].Stage, BindingShaderStage::Compute))
            {
                Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot));
            }
        }

        if(ComputeArgumentBuffer)
        {
            Encoder->setBuffer(ComputeArgumentBuffer->GetResource(), 0, BindGroupLayout->GetGroupNumber());
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


