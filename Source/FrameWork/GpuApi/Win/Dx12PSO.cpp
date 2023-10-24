#include "CommonHeader.h"
#include "Dx12PSO.h"
#include "Dx12Map.h"
#include "Dx12Device.h"
#include "Dx12RS.h"

namespace FRAMEWORK
{
    TRefCountPtr<Dx12Pso> CreateDx12Pso(const PipelineStateDesc& InPipelineStateDesc)
    {
        Dx12Shader* Vs = static_cast<Dx12Shader*>(InPipelineStateDesc.Vs);
        Dx12Shader* Ps = static_cast<Dx12Shader*>(InPipelineStateDesc.Ps);

		RootSignatureDesc RsDesc{};
		RsDesc.Layout0 = static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout0);
		RsDesc.Layout1 = static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout1);
		RsDesc.Layout2 = static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout2);
		RsDesc.Layout3 = static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout3);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};
		PsoDesc.pRootSignature = Dx12RootSignatureManager::GetRootSignature(RsDesc);
        PsoDesc.VS = { Vs->GetCompilationResult()->GetBufferPointer(), Vs->GetCompilationResult()->GetBufferSize() };
        PsoDesc.PS = { Ps->GetCompilationResult()->GetBufferPointer(), Ps->GetCompilationResult()->GetBufferSize() };
        PsoDesc.RasterizerState = MapRasterizerState(InPipelineStateDesc.RasterizerState);
        PsoDesc.BlendState = MapBlendState(InPipelineStateDesc.BlendState);
        PsoDesc.DepthStencilState.DepthEnable = false;
        PsoDesc.DepthStencilState.StencilEnable = false;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PsoDesc.NumRenderTargets = 1;
        PsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        PsoDesc.SampleDesc.Count = 1;
        PsoDesc.SampleDesc.Quality = 0;
        
        const uint32 RtFormatNum = InPipelineStateDesc.RtFormats.Num();
        for(uint32 i = 0 ; i < RtFormatNum; i++)
        {
            PsoDesc.RTVFormats[i] = MapTextureFormat(InPipelineStateDesc.RtFormats[i]);
        }

        TRefCountPtr<ID3D12PipelineState> Pso;
        DxCheck(GDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(Pso.GetInitReference())));
        return new Dx12Pso(MoveTemp(Pso));
    }
}
