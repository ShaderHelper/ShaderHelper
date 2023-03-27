#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	class Dx12Pso : public RenderPipelineState
	{
	public:
		Dx12Pso(TRefCountPtr<ID3D12PipelineState> InPipelineState) : PipelineState(MoveTemp(InPipelineState)) {}
		ID3D12PipelineState* GetResource() const { return PipelineState; }

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
	};

	D3D12_RASTERIZER_DESC MapRasterizerState(RasterizerStateDesc InDesc);
	D3D12_BLEND_DESC MapBlendState(BlendStateDesc InDesc);
	D3D12_BLEND MapBlendFactor(BlendFactor InFactor);
	D3D12_BLEND_OP MapBlendOp(BlendOp InOp);
	D3D12_CULL_MODE MapRasterizerCullMode(RasterizerCullMode InMode);
	D3D12_FILL_MODE MapRasterizerFillMode(RasterizerFillMode InMode);
}