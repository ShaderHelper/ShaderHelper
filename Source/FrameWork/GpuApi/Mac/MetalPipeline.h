#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
    class MetalPipelineState : public GpuPipelineState
    {
    public:
        MetalPipelineState(mtlpp::RenderPipelineState InPipelineState)
            : PipelineState(MoveTemp(InPipelineState))
        {}
        
    public:
        id<MTLRenderPipelineState> GetResource() const {
            return PipelineState.GetPtr();
        }
        
    private:
        mtlpp::RenderPipelineState PipelineState;
    };

    TRefCountPtr<MetalPipelineState> CreateMetalPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc);
}
