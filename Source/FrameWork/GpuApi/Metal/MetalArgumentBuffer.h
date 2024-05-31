#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"

namespace FRAMEWORK
{
    class MetalBindGroupLayout : public GpuBindGroupLayout
    {
    public:
        MetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
        id<MTLArgumentEncoder> GetVertexArgumentEncoder() const { 
            return (id<MTLArgumentEncoder>)VertexArgumentEncoder.get(); 
        }
        id<MTLArgumentEncoder> GetFragmentArgumentEncoder() const { 
            return (id<MTLArgumentEncoder>)FragmentArgumentEncoder.get(); 
        }
        
        MTLResourceUsage GetResourceUsage(BindingSlot InSlot) const {
            return ResourceUsages[InSlot];
        }
        
        MTLRenderStages GetRenderStages(BindingSlot InSlot) const {
            return RenderStages[InSlot];
        }
        
        
    private:
        MTLArgumentEncoderPtr VertexArgumentEncoder;
        MTLArgumentEncoderPtr FragmentArgumentEncoder;
        TMap<BindingSlot, MTLResourceUsage> ResourceUsages;
        TMap<BindingSlot, MTLRenderStages> RenderStages;
    };


    class MetalBindGroup : public GpuBindGroup
    {
    public:
        MetalBindGroup(const GpuBindGroupDesc& InDesc);
        void Apply(id<MTLRenderCommandEncoder> Encoder);
        
    private:
        TRefCountPtr<MetalBuffer> VertexArgumentBuffer;
        TRefCountPtr<MetalBuffer> FragmentArgumentBuffer;
        TMap<BindingSlot, id<MTLResource>> BindGroupResources;
    };

    TRefCountPtr<MetalBindGroupLayout> CreateMetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
    TRefCountPtr<MetalBindGroup> CreateMetalBindGroup(const GpuBindGroupDesc& Desc);
}
