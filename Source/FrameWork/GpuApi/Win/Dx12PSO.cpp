#include "CommonHeader.h"
#include "Dx12PSO.h"
#include "Dx12Map.h"
#include "Dx12Device.h"

namespace FRAMEWORK
{
    TRefCountPtr<Dx12Pso> CreateDx12Pso(const PipelineStateDesc& InPipelineStateDesc)
    {
        CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
        RootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        TRefCountPtr<ID3DBlob> Signature;
        TRefCountPtr<ID3DBlob> Error;
        TRefCountPtr<ID3D12RootSignature> RootSignature;
        DxCheck(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, Signature.GetInitReference(), Error.GetInitReference()));
        DxCheck(GDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(RootSignature.GetInitReference())));
        //TODO RootSignature Manager.

        Dx12Shader* Vs = static_cast<Dx12Shader*>(InPipelineStateDesc.Vs);
        Dx12Shader* Ps = static_cast<Dx12Shader*>(InPipelineStateDesc.Ps);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};
        PsoDesc.pRootSignature = RootSignature;
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
        return new Dx12Pso(MoveTemp(Pso), MoveTemp(RootSignature));
    }
}
