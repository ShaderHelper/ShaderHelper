#include "CommonHeader.h"
#include "Dx12PSO.h"
#include "Dx12Map.h"
#include "Dx12Device.h"
#include "Dx12RS.h"

namespace FRAMEWORK
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE MapTopologyType(D3D_PRIMITIVE_TOPOLOGY InTopology)
	{
		switch (InTopology)
		{
		case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
		case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		default:
			AUX::Unreachable();
		}
	}

    TRefCountPtr<Dx12RenderPso> CreateDx12RenderPso(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
    {
		const uint32 TargetNum = InPipelineStateDesc.Targets.Num();

        Dx12Shader* Vs = static_cast<Dx12Shader*>(InPipelineStateDesc.Vs);
        Dx12Shader* Ps = static_cast<Dx12Shader*>(InPipelineStateDesc.Ps);

		RootSignatureDesc RsDesc{
			static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout0), static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout1),
			static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout2), static_cast<Dx12BindGroupLayout*>(InPipelineStateDesc.BindGroupLayout3)
		};

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};
		PsoDesc.pRootSignature = Dx12RootSignatureManager::GetRootSignature(RsDesc)->GetResource();
        PsoDesc.VS = { Vs->GetCompilationResult()->GetBufferPointer(), Vs->GetCompilationResult()->GetBufferSize() };
		if (Ps)
		{
			PsoDesc.PS = { Ps->GetCompilationResult()->GetBufferPointer(), Ps->GetCompilationResult()->GetBufferSize() };
		}

        PsoDesc.RasterizerState = MapRasterizerState(InPipelineStateDesc.RasterizerState);
        PsoDesc.DepthStencilState.DepthEnable = false;
        PsoDesc.DepthStencilState.StencilEnable = false;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = MapTopologyType(MapPrimitiveType(InPipelineStateDesc.Primitive));
        PsoDesc.NumRenderTargets = TargetNum;
        PsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        PsoDesc.SampleDesc.Count = 1;
        PsoDesc.SampleDesc.Quality = 0;
        
		CD3DX12_BLEND_DESC BlendDesc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT{});
        for(uint32 i = 0 ; i < TargetNum; i++)
        {
            PsoDesc.RTVFormats[i] = MapTextureFormat(InPipelineStateDesc.Targets[i].TargetFormat);

			BlendDesc.RenderTarget[i].BlendEnable = InPipelineStateDesc.Targets[i].BlendEnable;
			BlendDesc.RenderTarget[i].SrcBlend = MapBlendFactor(InPipelineStateDesc.Targets[i].SrcFactor);
			BlendDesc.RenderTarget[i].SrcBlendAlpha = MapBlendFactor(InPipelineStateDesc.Targets[i].SrcAlphaFactor);
			BlendDesc.RenderTarget[i].DestBlend = MapBlendFactor(InPipelineStateDesc.Targets[i].DestFactor);
			BlendDesc.RenderTarget[i].DestBlendAlpha = MapBlendFactor(InPipelineStateDesc.Targets[i].DestAlphaFactor);
			BlendDesc.RenderTarget[i].BlendOp = MapBlendOp(InPipelineStateDesc.Targets[i].ColorOp);
			BlendDesc.RenderTarget[i].BlendOpAlpha = MapBlendOp(InPipelineStateDesc.Targets[i].AlphaOp);
			BlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE(uint32(InPipelineStateDesc.Targets[i].Mask));
	
        }
		PsoDesc.BlendState = MoveTemp(BlendDesc);

        TRefCountPtr<ID3D12PipelineState> Pso;
        DxCheck(GDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(Pso.GetInitReference())));
        return new Dx12RenderPso(MoveTemp(Pso), MapPrimitiveType(InPipelineStateDesc.Primitive));
    }
}
