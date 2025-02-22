#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"
#include "MetalTexture.h"
#include "MetalGpuRhiBackend.h"

namespace FW
{

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
        for(const auto& [Slot, LayoutBindingEntry]  : Desc.Layouts)
        {
            RenderStages.Add(Slot, MapShaderVisibility(LayoutBindingEntry.Stage));
            
            MTLArgumentDescriptor* ArgDesc = [MTLArgumentDescriptor argumentDescriptor];
            ArgDesc.index = Slot;
            if(LayoutBindingEntry.Type == BindingType::UniformBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::Texture)
            {
                ArgDesc.dataType = MTLDataTypeTexture;
                ArgDesc.textureType = MTLTextureType2D;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else if(LayoutBindingEntry.Type == BindingType::Sampler)
            {
                ArgDesc.dataType = MTLDataTypeSampler;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(Slot, MTLResourceUsageRead);
            }
            else
            {
                check(false);
            }
            
            if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Vertex))
            {
                VertexArgDescs.Add(ArgDesc);
            }
            
            if(EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Pixel))
            {
                FragmentArgDescs.Add(ArgDesc);
            }
        }
        
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

    }

    MetalBindGroup::MetalBindGroup(const GpuBindGroupDesc& InDesc)
        : GpuBindGroup(InDesc)
    {
        GDeferredReleaseOneFrame.Add(this);
        
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(InDesc.Layout);
        MTL::ArgumentEncoder* VertexArgumentEncoder = BindGroupLayout->GetVertexArgumentEncoder();
        MTL::ArgumentEncoder* FragmentArgumentEncoder = BindGroupLayout->GetFragmentArgumentEncoder();
        
        if(VertexArgumentEncoder != nullptr)
        {
            VertexArgumentBuffer = CreateMetalBuffer(VertexArgumentEncoder->encodedLength(), GpuBufferUsage::Dynamic);
            VertexArgumentEncoder->setArgumentBuffer(VertexArgumentBuffer->GetResource(), 0);
        }
        
        if(FragmentArgumentEncoder != nullptr)
        {
            FragmentArgumentBuffer = CreateMetalBuffer(FragmentArgumentEncoder->encodedLength(), GpuBufferUsage::Dynamic);
            FragmentArgumentEncoder->setArgumentBuffer(FragmentArgumentBuffer->GetResource(), 0);
        }
        
        for(const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
        {
            MTLRenderStages Stage = BindGroupLayout->GetRenderStages(Slot);
            if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    VertexArgumentEncoder->setBuffer(Buffer->GetResource(), 0, Slot);
                }
                
                if(Stage & MTLRenderStageFragment)
                {
                    FragmentArgumentEncoder->setBuffer(Buffer->GetResource(), 0, Slot);
                }
                BindGroupResources.Add(Slot, Buffer->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    VertexArgumentEncoder->setTexture(Tex->GetResource(), Slot);
                }
                
                if(Stage & MTLRenderStageFragment)
                {
                    FragmentArgumentEncoder->setTexture(Tex->GetResource(), Slot);
                }
                BindGroupResources.Add(Slot, Tex->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Sampler)
            {
                MetalSampler* Sampler = static_cast<MetalSampler*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    VertexArgumentEncoder->setSamplerState(Sampler->GetResource(), Slot);
                }
                if(Stage & MTLRenderStageFragment)
                {
                    FragmentArgumentEncoder->setSamplerState(Sampler->GetResource(), Slot);
                }
            }
            else
            {
                check(false);
            }
        }
    }

    void MetalBindGroup::Apply(MTL::RenderCommandEncoder* Encoder)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            Encoder->useResource(MtlResource, BindGroupLayout->GetResourceUsage(Slot), BindGroupLayout->GetRenderStages(Slot));
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

    TRefCountPtr<MetalBindGroupLayout> CreateMetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
    {
        return new MetalBindGroupLayout(LayoutDesc);
    }

    TRefCountPtr<MetalBindGroup> CreateMetalBindGroup(const GpuBindGroupDesc& Desc)
    {
        return new MetalBindGroup(Desc);
    }
}


