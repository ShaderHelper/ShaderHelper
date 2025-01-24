#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
    class MetalRenderPipelineState : public GpuPipelineState
    {
    public:
        MetalRenderPipelineState(MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType)
            : PipelineState(MoveTemp(InPipelineState))
            , PrimitiveType(InPrimitiveType)
        {}
        
    public:
        MTL::RenderPipelineState* GetResource() const {
            return PipelineState.get();
        }
        
        MTLPrimitiveType GetPrimitiveType() const {
            return PrimitiveType;
        }
        
    private:
        MTLRenderPipelineStatePtr PipelineState;
        MTLPrimitiveType PrimitiveType;
    };

    TRefCountPtr<MetalRenderPipelineState> CreateMetalRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
}
