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
        id<MTLArgumentEncoder> GetVertexArgumentEncoder() const { return VertexArgumentEncoder; }
        id<MTLArgumentEncoder> GetFragmentArgumentEncoder() const { return FragmentArgumentEncoder; }
        
        MTLResourceUsage GetResourceUsage(BindingSlot InSlot) const {
            return ResourceUsages[InSlot];
        }
        
        MTLRenderStages GetRenderStages(BindingSlot InSlot) const {
            return RenderStages[InSlot];
        }
        
        
    private:
        mtlpp::ArgumentEncoder VertexArgumentEncoder;
        mtlpp::ArgumentEncoder FragmentArgumentEncoder;
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
