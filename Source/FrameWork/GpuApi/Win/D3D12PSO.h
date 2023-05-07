#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"
#include "D3D12Shader.h"

namespace FRAMEWORK
{
	class Dx12Pso : public GpuPipelineState
	{
	public:
		Dx12Pso(TRefCountPtr<ID3D12PipelineState> InPipelineState)
			: PipelineState(MoveTemp(InPipelineState))
		{}
        
    public:
		ID3D12PipelineState* GetResource() const { return PipelineState; }

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
	};

    TRefCountPtr<Dx12Pso> CreateDx12Pso(const PipelineStateDesc& InPipelineStateDesc);
}
