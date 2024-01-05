#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
    class MetalPipelineState : public GpuPipelineState
    {
    public:
        MetalPipelineState(mtlpp::RenderPipelineState InPipelineState, MTLPrimitiveType InPrimitiveType)
            : PipelineState(MoveTemp(InPipelineState))
            , PrimitiveType(InPrimitiveType)
        {}
        
    public:
        id<MTLRenderPipelineState> GetResource() const {
            return PipelineState.GetPtr();
        }
        
        MTLPrimitiveType GetPrimitiveType() const {
            return PrimitiveType;
        }
        
    private:
        mtlpp::RenderPipelineState PipelineState;
        MTLPrimitiveType PrimitiveType;
    };

    TRefCountPtr<MetalPipelineState> CreateMetalPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc);
}
