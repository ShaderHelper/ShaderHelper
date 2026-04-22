#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"
#include "MetalBuffer.h"
#include "MetalPipeline.h"

namespace FW
{
    class MetalBindGroupLayout : public GpuBindGroupLayout
    {
    public:
        MetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
        
        MTLResourceUsage GetResourceUsage(BindingSlot InSlot) const {
            return ResourceUsages[InSlot];
        }
        MTLRenderStages GetRenderStages(BindingSlot InSlot) const {
            return RenderStages[InSlot];
        }

    private:
        TMap<BindingSlot, MTLResourceUsage> ResourceUsages;
        TMap<BindingSlot, MTLRenderStages> RenderStages;
    };

    class MetalBindGroup : public GpuBindGroup
    {
    public:
        MetalBindGroup(const GpuBindGroupDesc& InDesc);

        void Apply(MTL::RenderCommandEncoder* Encoder, MetalRenderPipelineState* Pipeline);
        void Apply(MTL::ComputeCommandEncoder* Encoder, MetalComputePipelineState* Pipeline);
        
    private:
		void EncodeResources(const MetalFunctionArgEncoder& StageEnc, BindingShaderStage Stage) const;
        MetalBuffer* GetOrCreateArgBuffer(const MetalFunctionArgEncoder& StageEnc);

        TMap<BindingSlot, MTL::Resource*> BindGroupResources;
        TMap<MTL::ArgumentEncoder*, TRefCountPtr<MetalBuffer>> ArgBufferCache;
    };

    TRefCountPtr<MetalBindGroupLayout> CreateMetalBindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
    TRefCountPtr<MetalBindGroup> CreateMetalBindGroup(const GpuBindGroupDesc& Desc);
}
