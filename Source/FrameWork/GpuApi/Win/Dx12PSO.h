#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Shader.h"

namespace FRAMEWORK
{
	class Dx12Pso : public GpuPipelineState
	{
	public:
		Dx12Pso(TRefCountPtr<ID3D12PipelineState> InPipelineState, TRefCountPtr<ID3D12RootSignature> InRootSig)
			: PipelineState(MoveTemp(InPipelineState))
			, RootSignature(MoveTemp(InRootSig))
		{}
        
    public:
		ID3D12PipelineState* GetResource() const { return PipelineState; }
		ID3D12RootSignature* GetRootSig() const {
			return RootSignature;
		}

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
		TRefCountPtr<ID3D12RootSignature> RootSignature;
	};

    TRefCountPtr<Dx12Pso> CreateDx12Pso(const PipelineStateDesc& InPipelineStateDesc);
}
