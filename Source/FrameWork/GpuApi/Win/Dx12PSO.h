#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Shader.h"
#include "Dx12Util.h"

namespace FRAMEWORK
{
	class Dx12Pso : public GpuPipelineState, public Dx12DeferredDeleteObject<Dx12Pso>
	{
	public:
		Dx12Pso(TRefCountPtr<ID3D12PipelineState> InPipelineState, D3D_PRIMITIVE_TOPOLOGY InPritimiveTopology)
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

    TRefCountPtr<Dx12Pso> CreateDx12Pso(const GpuPipelineStateDesc& InPipelineStateDesc);
}
