#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"
#include "D3D12Shader.h"

namespace FRAMEWORK
{
	class Dx12Pso : public RenderPipelineState
	{
	public:
		Dx12Pso(TRefCountPtr<ID3D12PipelineState> InPipelineState, 
			TRefCountPtr<ID3D12RootSignature> InRootSig, 
			TRefCountPtr<Dx12Shader> InVs, TRefCountPtr<Dx12Shader> InPs) 
			: PipelineState(MoveTemp(InPipelineState)) 
			, RootSig(MoveTemp(InRootSig))
			, Vs(MoveTemp(InVs))
			, Ps(MoveTemp(InPs))
		{}
        
    public:
		ID3D12PipelineState* GetResource() const { return PipelineState; }
		ID3D12RootSignature* GetRootSig() const { return RootSig; }

	private:
		TRefCountPtr<ID3D12PipelineState> PipelineState;
		TRefCountPtr<ID3D12RootSignature> RootSig;
		TRefCountPtr<Dx12Shader> Vs;
		TRefCountPtr<Dx12Shader> Ps;
	};
}
