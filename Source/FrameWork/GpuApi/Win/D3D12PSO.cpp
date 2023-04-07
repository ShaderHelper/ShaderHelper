#include "CommonHeader.h"
#include "D3D12PSO.h"

namespace FRAMEWORK
{
	D3D12_BLEND MapBlendFactor(BlendFactor InFactor)
	{
		switch (InFactor)
		{
		case BlendFactor::Zero:			return D3D12_BLEND_ZERO;
		case BlendFactor::One:			return D3D12_BLEND_ONE;
		case BlendFactor::SrcColor:		return D3D12_BLEND_SRC_COLOR;
		case BlendFactor::InvSrcColor:	return D3D12_BLEND_INV_SRC_COLOR;
		case BlendFactor::DestColor:	return D3D12_BLEND_DEST_COLOR;
		case BlendFactor::InvDestColor:	return D3D12_BLEND_INV_DEST_COLOR;
		case BlendFactor::SrcAlpha:		return D3D12_BLEND_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:	return D3D12_BLEND_INV_SRC_ALPHA;
		case BlendFactor::DestAlpha:	return D3D12_BLEND_DEST_ALPHA;
		case BlendFactor::InvDestAlpha:	return D3D12_BLEND_INV_DEST_ALPHA;
		default:
			SH_LOG(LogDx12, Fatal, TEXT("Invalid BlendFactor."));
			return D3D12_BLEND_ZERO;
		}
	}

	D3D12_BLEND_OP MapBlendOp(BlendOp InOp)
	{
		switch (InOp)
		{
		case BlendOp::Add:			return D3D12_BLEND_OP_ADD;
		case BlendOp::Substract:	return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::Min:			return D3D12_BLEND_OP_MIN;
		case BlendOp::Max:			return D3D12_BLEND_OP_MAX;
		default:
			SH_LOG(LogDx12, Fatal, TEXT("Invalid BlendOp."));
			return D3D12_BLEND_OP_ADD;
		}
	}

	D3D12_CULL_MODE MapRasterizerCullMode(RasterizerCullMode InMode)
	{
		switch (InMode)
		{
		case RasterizerCullMode::None:	return D3D12_CULL_MODE_NONE;
		case RasterizerCullMode::Front:	return D3D12_CULL_MODE_FRONT;
		case RasterizerCullMode::Back:	return D3D12_CULL_MODE_BACK;
		default:
			SH_LOG(LogDx12, Fatal, TEXT("Invalid CullMode."));
			return D3D12_CULL_MODE_NONE;
		}
	}

	D3D12_FILL_MODE MapRasterizerFillMode(RasterizerFillMode InMode)
	{
		switch (InMode)
		{
		case RasterizerFillMode::WireFrame:	return D3D12_FILL_MODE_WIREFRAME;
		case RasterizerFillMode::Solid:		return D3D12_FILL_MODE_SOLID;
		default:
			SH_LOG(LogDx12, Fatal, TEXT("Invalid CullMode."));
			return D3D12_FILL_MODE_SOLID;
		}
	}
	
	D3D12_RASTERIZER_DESC MapRasterizerState(RasterizerStateDesc InDesc)
	{
		CD3DX12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT{});
		Desc.FillMode = MapRasterizerFillMode(InDesc.FillMode);
		Desc.CullMode = MapRasterizerCullMode(InDesc.CullMode);
		return Desc;
	}

	D3D12_BLEND_DESC MapBlendState(BlendStateDesc InDesc)
	{
		CD3DX12_BLEND_DESC Desc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT{});
		const uint32 BlendRtNum = InDesc.RtDesc.Num();
		for (uint32 i = 0; i < BlendRtNum; i++)
		{
			Desc.RenderTarget[i].BlendEnable = InDesc.RtDesc[i].BlendEnable;
			Desc.RenderTarget[i].SrcBlend = MapBlendFactor(InDesc.RtDesc[i].SrcFactor);
			Desc.RenderTarget[i].SrcBlendAlpha = MapBlendFactor(InDesc.RtDesc[i].SrcAlphaFactor);
			Desc.RenderTarget[i].DestBlend = MapBlendFactor(InDesc.RtDesc[i].DestFactor);
			Desc.RenderTarget[i].DestBlendAlpha = MapBlendFactor(InDesc.RtDesc[i].DestAlphaFactor);
			Desc.RenderTarget[i].BlendOp = MapBlendOp(InDesc.RtDesc[i].ColorOp);
			Desc.RenderTarget[i].BlendOpAlpha = MapBlendOp(InDesc.RtDesc[i].AlphaOp);
			Desc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE(D3D12_COLOR_WRITE_ENABLE_ALL ^ uint32(InDesc.RtDesc[i].Mask));
		}
		return Desc;
	}

}