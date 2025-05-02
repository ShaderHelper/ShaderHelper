#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Shader.h"
#include "Dx12Util.h"

namespace FW
{
	class Dx12RenderPso : public GpuRenderPipelineState, public Dx12DeferredDeleteObject<Dx12RenderPso>
	{
	public:
		Dx12RenderPso(TRefCountPtr<ID3D12PipelineState> InPipelineState, D3D_PRIMITIVE_TOPOLOGY InPritimiveTopology)
			: PipelineState(MoveTemp(InPipelineState))
			, PritimiveTopology(InPritimiveTopology)
		{}
        
    public:
		ID3D12PipelineState* GetResource() const { return PipelineState; }
		D3D_PRIMITIVE_TOPOLOGY GetPritimiveTopology() const { return PritimiveTopology; }

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
		D3D_PRIMITIVE_TOPOLOGY PritimiveTopology;
	};

	class Dx12ComputePso : public GpuComputePipelineState, public Dx12DeferredDeleteObject<Dx12ComputePso>
	{
	public:
		Dx12ComputePso(TRefCountPtr<ID3D12PipelineState> InPipelineState)
			: PipelineState(MoveTemp(InPipelineState))
		{ }

	public:
		ID3D12PipelineState* GetResource() const { return PipelineState; }

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
	};

    TRefCountPtr<Dx12RenderPso> CreateDx12RenderPso(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
	TRefCountPtr<Dx12ComputePso> CreateDx12ComputePso(const GpuComputePipelineStateDesc& InPipelineStateDesc);
}
