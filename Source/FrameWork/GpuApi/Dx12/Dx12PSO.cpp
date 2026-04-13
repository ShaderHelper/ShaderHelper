#include "CommonHeader.h"
#include "Dx12PSO.h"
#include "Dx12Map.h"
#include "Dx12Device.h"
#include "Dx12RS.h"
#include "GpuApi/GpuApiValidation.h"

namespace FW
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

		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Vs);
			if (Ps)
			{
				CheckShaderLayoutBinding(InPipelineStateDesc, Ps);
			}
		}

		RootSignatureDesc RsDesc{InPipelineStateDesc.BindGroupLayouts};

		TArray<TArray<ANSICHAR>> SemanticNameStorage;
		TArray<D3D12_INPUT_ELEMENT_DESC> InputElements;
		for (int32 BufferSlot = 0; BufferSlot < InPipelineStateDesc.VertexLayout.Num(); ++BufferSlot)
		{
			const GpuVertexLayoutDesc& BufferLayout = InPipelineStateDesc.VertexLayout[BufferSlot];
			for (const GpuVertexAttributeDesc& Attribute : BufferLayout.Attributes)
			{
				checkf(!Attribute.SemanticName.IsEmpty(), TEXT("DX12 vertex attribute semantic name must not be empty."));
				auto& Storage = SemanticNameStorage.AddDefaulted_GetRef();
				FTCHARToUTF8 SemanticNameUtf8(*Attribute.SemanticName);
				Storage.Append(reinterpret_cast<const ANSICHAR*>(SemanticNameUtf8.Get()), SemanticNameUtf8.Length());
				Storage.Add('\0');
				InputElements.Add({
					.SemanticName = Storage.GetData(),
					.SemanticIndex = Attribute.SemanticIndex,
					.Format = MapTextureFormat(Attribute.Format),
					.InputSlot = static_cast<uint32>(BufferSlot),
					.AlignedByteOffset = Attribute.ByteOffset,
					.InputSlotClass = MapVertexStepMode(BufferLayout.StepMode),
					.InstanceDataStepRate = BufferLayout.StepMode == GpuVertexStepMode::Instance ? 1u : 0u,
				});
			}
		}

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};
		PsoDesc.pRootSignature = Dx12RootSignatureManager::GetRootSignature(RsDesc)->GetResource();
        PsoDesc.VS = { Vs->GetCompilationResult()->GetBufferPointer(), Vs->GetCompilationResult()->GetBufferSize() };
		if (Ps)
		{
			PsoDesc.PS = { Ps->GetCompilationResult()->GetBufferPointer(), Ps->GetCompilationResult()->GetBufferSize() };
		}
		PsoDesc.InputLayout = { InputElements.GetData(), (uint32)InputElements.Num() };

        PsoDesc.RasterizerState = MapRasterizerState(InPipelineStateDesc.RasterizerState);
        PsoDesc.DepthStencilState.DepthEnable = InPipelineStateDesc.DepthStencilState.IsSet();
        if (InPipelineStateDesc.DepthStencilState)
        {
            PsoDesc.DepthStencilState.DepthWriteMask = InPipelineStateDesc.DepthStencilState->DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            PsoDesc.DepthStencilState.DepthFunc = MapComparisonFunc(InPipelineStateDesc.DepthStencilState->DepthCompare);
            PsoDesc.DSVFormat = MapTextureFormat(InPipelineStateDesc.DepthStencilState->DepthFormat);
        }
        PsoDesc.DepthStencilState.StencilEnable = false;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = MapTopologyType(MapPrimitiveType(InPipelineStateDesc.Primitive));
        PsoDesc.NumRenderTargets = TargetNum;
		PsoDesc.SampleDesc.Count = InPipelineStateDesc.SampleCount;
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
        return new Dx12RenderPso(InPipelineStateDesc, MoveTemp(Pso), MapPrimitiveType(InPipelineStateDesc.Primitive));
    }

	TRefCountPtr<Dx12ComputePso> CreateDx12ComputePso(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		Dx12Shader* Cs = static_cast<Dx12Shader*>(InPipelineStateDesc.Cs);
		if (InPipelineStateDesc.CheckLayout)
		{
			CheckShaderLayoutBinding(InPipelineStateDesc, Cs);
		}

		RootSignatureDesc RsDesc{InPipelineStateDesc.BindGroupLayouts};

		D3D12_COMPUTE_PIPELINE_STATE_DESC PsoDesc{};
		PsoDesc.CS = { Cs->GetCompilationResult()->GetBufferPointer(), Cs->GetCompilationResult()->GetBufferSize() };
		PsoDesc.pRootSignature = Dx12RootSignatureManager::GetRootSignature(RsDesc)->GetResource();

		TRefCountPtr<ID3D12PipelineState> Pso;
		DxCheck(GDevice->CreateComputePipelineState(&PsoDesc, IID_PPV_ARGS(Pso.GetInitReference())));
		
		return new Dx12ComputePso(InPipelineStateDesc, MoveTemp(Pso));
	}
}
