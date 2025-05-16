#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
    class MetalRenderPipelineState : public GpuRenderPipelineState
    {
    public:
        MetalRenderPipelineState(GpuRenderPipelineStateDesc InDesc, MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType);
        
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
        MetalComputePipelineState(GpuComputePipelineStateDesc InDesc, MTLComputePipelineStatePtr InPipelineState);
		MTL::ComputePipelineState* GetResource() const {
			return PipelineState.get();
		}
		
    private:
        MTLComputePipelineStatePtr PipelineState;
    };

    TRefCountPtr<MetalRenderPipelineState> CreateMetalRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
    TRefCountPtr<MetalComputePipelineState> CreateMetalComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);
}
