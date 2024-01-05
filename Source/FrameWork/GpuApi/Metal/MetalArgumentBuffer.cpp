#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"
#include "MetalTexture.h"

namespace FRAMEWORK
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
            VertexArgumentEncoder = GDevice.NewArgumentEncoderWithArguments([NSArray arrayWithObjects:VertexArgDescs.GetData() count:VertexArgDescs.Num()]);
        }
        
        if(FragmentArgDescs.Num() > 0)
        {
            FragmentArgumentEncoder = GDevice.NewArgumentEncoderWithArguments([NSArray arrayWithObjects:FragmentArgDescs.GetData() count:FragmentArgDescs.Num()]);
        }

    }

    MetalBindGroup::MetalBindGroup(const GpuBindGroupDesc& InDesc)
        : GpuBindGroup(InDesc)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(InDesc.Layout);
        id<MTLArgumentEncoder> VertexArgumentEncoder = BindGroupLayout->GetVertexArgumentEncoder();
        id<MTLArgumentEncoder> FragmentArgumentEncoder = BindGroupLayout->GetFragmentArgumentEncoder();
        
        if(VertexArgumentEncoder != nullptr)
        {
            VertexArgumentBuffer = CreateMetalBuffer(VertexArgumentEncoder.encodedLength, GpuBufferUsage::Dynamic);
            [VertexArgumentEncoder setArgumentBuffer:VertexArgumentBuffer->GetResource() offset:0];
        }
        
        if(FragmentArgumentEncoder != nullptr)
        {
            FragmentArgumentBuffer = CreateMetalBuffer(FragmentArgumentEncoder.encodedLength, GpuBufferUsage::Dynamic);
            [FragmentArgumentEncoder setArgumentBuffer:FragmentArgumentBuffer->GetResource() offset:0];
        }
        
        for(const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
        {
            MTLRenderStages Stage = BindGroupLayout->GetRenderStages(Slot);
            if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    [VertexArgumentEncoder setBuffer:Buffer->GetResource() offset:0 atIndex:Slot];
                }
                
                if(Stage & MTLRenderStageFragment)
                {
                    [FragmentArgumentEncoder setBuffer:Buffer->GetResource() offset:0 atIndex:Slot];
                }
                BindGroupResources.Add(Slot, Buffer->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Texture)
            {
                MetalTexture* Tex = static_cast<MetalTexture*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    [VertexArgumentEncoder setTexture:Tex->GetResource() atIndex:Slot];
                }
                
                if(Stage & MTLRenderStageFragment)
                {
                    [FragmentArgumentEncoder setTexture:Tex->GetResource() atIndex:Slot];
                }
                BindGroupResources.Add(Slot, Tex->GetResource());
            }
            else if(ResourceBindingEntry.Resource->GetType() == GpuResourceType::Sampler)
            {
                MetalSampler* Sampler = static_cast<MetalSampler*>(ResourceBindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    [VertexArgumentEncoder setSamplerState:Sampler->GetResource() atIndex:Slot];
                }
                if(Stage & MTLRenderStageFragment)
                {
                    [FragmentArgumentEncoder setSamplerState:Sampler->GetResource() atIndex:Slot];
                }
            }
            else
            {
                check(false);
            }
        }
    }

    void MetalBindGroup::Apply(id<MTLRenderCommandEncoder> Encoder)
    {
        MetalBindGroupLayout* BindGroupLayout = static_cast<MetalBindGroupLayout*>(GetLayout());
        for(auto [Slot ,MtlResource] : BindGroupResources)
        {
            [Encoder useResource:MtlResource usage:BindGroupLayout->GetResourceUsage(Slot) stages:BindGroupLayout->GetRenderStages(Slot)];
        }
        
        if(VertexArgumentBuffer)
        {
            [Encoder setVertexBuffer:VertexArgumentBuffer->GetResource() offset:0 atIndex:BindGroupLayout->GetGroupNumber()];
        }
        
        if(FragmentArgumentBuffer)
        {
            [Encoder setFragmentBuffer:FragmentArgumentBuffer->GetResource() offset:0 atIndex:BindGroupLayout->GetGroupNumber()];
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


