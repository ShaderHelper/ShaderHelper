#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
    class MetalRenderPipelineState : public GpuRenderPipelineState
    {
    public:
        MetalRenderPipelineState(GpuRenderPipelineStateDesc InDesc, MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType, MTLDepthStencilStatePtr InDepthStencilState);
        
    public:
        MTL::RenderPipelineState* GetResource() const {
            return PipelineState.get();
        }
        
        MTLPrimitiveType GetPrimitiveType() const {
            return PrimitiveType;
        }

        MTL::DepthStencilState* GetDepthStencilState() const {
            return DepthStencilState.get();
        }
        
    private:
        MTLRenderPipelineStatePtr PipelineState;
        MTLPrimitiveType PrimitiveType;
        MTLDepthStencilStatePtr DepthStencilState;
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
