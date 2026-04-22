#pragma once
#include "MetalCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
    // Bundles a per-stage argument encoder with the set of argument-buffer indices
    // that the compiled function actually declares.  EncodeResources uses this set
    // to skip any slot not present in the shader, avoiding mismatch.
    struct MetalFunctionArgEncoder
    {
        MTLArgumentEncoderPtr  Encoder;
        TSet<int32>            ValidArgIndices;
    };

    class MetalRenderPipelineState : public GpuRenderPipelineState
    {
    public:
        MetalRenderPipelineState(GpuRenderPipelineStateDesc InDesc, MTLRenderPipelineStatePtr InPipelineState, MTLPrimitiveType InPrimitiveType, MTLDepthStencilStatePtr InDepthStencilState,
            TMap<int32, MetalFunctionArgEncoder> InVsArgumentEncoders, TMap<int32, MetalFunctionArgEncoder> InFsArgumentEncoders);
        
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

        const MetalFunctionArgEncoder* GetVsArgumentEncoder(int32 SetNumber) const {
            return VsArgumentEncoders.Find(SetNumber);
        }

        const MetalFunctionArgEncoder* GetFsArgumentEncoder(int32 SetNumber) const {
            return FsArgumentEncoders.Find(SetNumber);
        }
        
    private:
        MTLRenderPipelineStatePtr PipelineState;
        MTLPrimitiveType PrimitiveType;
        MTLDepthStencilStatePtr DepthStencilState;

        // Keyed by BindingGroupSlot (= [[buffer(N)]] index).
        TMap<int32, MetalFunctionArgEncoder> VsArgumentEncoders;
        TMap<int32, MetalFunctionArgEncoder> FsArgumentEncoders;
    };

    class MetalComputePipelineState : public GpuComputePipelineState
    {
    public:
        MetalComputePipelineState(GpuComputePipelineStateDesc InDesc, MTLComputePipelineStatePtr InPipelineState,
            TMap<int32, MetalFunctionArgEncoder> InCsArgumentEncoders);
		MTL::ComputePipelineState* GetResource() const {
			return PipelineState.get();
		}

        const MetalFunctionArgEncoder* GetCsArgumentEncoder(int32 SetNumber) const {
            return CsArgumentEncoders.Find(SetNumber);
        }
		
    private:
        MTLComputePipelineStatePtr PipelineState;

        TMap<int32, MetalFunctionArgEncoder> CsArgumentEncoders;
    };

    TRefCountPtr<MetalRenderPipelineState> CreateMetalRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
    TRefCountPtr<MetalComputePipelineState> CreateMetalComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);
}
