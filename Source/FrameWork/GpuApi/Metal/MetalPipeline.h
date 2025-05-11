#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
    class MetalRenderPipelineState : public GpuRenderPipelineState
    {
    public:
        MetalRenderPipelineState(MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType);
        
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

    class MetalComputePipelineState : public GpuComputePipelineState
    {
    public:
        MetalComputePipelineState(MTLComputePipelineStatePtr InPipelineState);
        
    private:
        MTLComputePipelineStatePtr PipelineState;
    };

    TRefCountPtr<MetalRenderPipelineState> CreateMetalRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
    TRefCountPtr<MetalComputePipelineState> CreateMetalComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);
}
