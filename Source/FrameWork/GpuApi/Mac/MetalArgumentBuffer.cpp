#include "CommonHeader.h"
#include "MetalDevice.h"
#include "MetalArgumentBuffer.h"

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
        for(const auto& BindingLayoutEntry : Desc.Layouts)
        {
            MTLArgumentDescriptor* ArgDesc = [MTLArgumentDescriptor argumentDescriptor];
            ArgDesc.index = BindingLayoutEntry.Slot;
            if(BindingLayoutEntry.Type == BindingType::UniformBuffer)
            {
                ArgDesc.dataType = MTLDataTypePointer;
                ArgDesc.access = MTLArgumentAccessReadOnly;
                ResourceUsages.Add(BindingLayoutEntry.Slot, MTLResourceUsageRead);
                RenderStages.Add(BindingLayoutEntry.Slot, MapShaderVisibility(BindingLayoutEntry.Stage));
            }
            else
            {
                //TODO
                check(false);
            }
            
            if(EnumHasAnyFlags(BindingLayoutEntry.Stage, BindingShaderStage::Vertex))
            {
                VertexArgDescs.Add(ArgDesc);
            }
            
            if(EnumHasAnyFlags(BindingLayoutEntry.Stage, BindingShaderStage::Pixel))
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
        
        for(const auto& BindingEntry : InDesc.Resources)
        {
            MTLRenderStages Stage = BindGroupLayout->GetRenderStages(BindingEntry.Slot);
            if(BindingEntry.Resource->GetType() == GpuResourceType::Buffer)
            {
                MetalBuffer* Buffer = static_cast<MetalBuffer*>(BindingEntry.Resource);
                if(Stage & MTLRenderStageVertex)
                {
                    [VertexArgumentEncoder setBuffer:Buffer->GetResource() offset:0 atIndex:BindingEntry.Slot];
                }
                
                if(Stage & MTLRenderStageFragment)
                {
                    [FragmentArgumentEncoder setBuffer:Buffer->GetResource() offset:0 atIndex:BindingEntry.Slot];
                }
                BindGroupResources.Add(BindingEntry.Slot, Buffer->GetResource());
            }
            else
            {
                
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


