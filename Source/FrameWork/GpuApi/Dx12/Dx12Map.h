#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"

namespace FW
{

	inline D3D12_RESOURCE_STATES MapResourceState(GpuResourceState InResourceState)
	{
		switch (InResourceState)
		{
		case GpuResourceState::RenderTargetWrite:                return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case GpuResourceState::CopyDst:                          return D3D12_RESOURCE_STATE_COPY_DEST;
		case GpuResourceState::UnorderedAccess:                  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		default:
			check(!EnumHasAnyFlags(InResourceState, GpuResourceState::WriteMask));
			D3D12_RESOURCE_STATES State{};

			if (EnumHasAnyFlags(InResourceState, GpuResourceState::UniformBuffer))
			{
				State |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			}
			if (EnumHasAnyFlags(InResourceState, GpuResourceState::ShaderResourceRead))
			{
				State |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			}
			if (EnumHasAnyFlags(InResourceState, GpuResourceState::CopySrc))
			{
				State |= D3D12_RESOURCE_STATE_COPY_SOURCE;
			}

			check(State != D3D12_RESOURCE_STATE_COMMON);
			return State;
		}
	}

    inline D3D12_BLEND MapBlendFactor(BlendFactor InFactor)
    {
        switch (InFactor)
        {
        case BlendFactor::Zero:            return D3D12_BLEND_ZERO;
        case BlendFactor::One:             return D3D12_BLEND_ONE;
        case BlendFactor::SrcColor:        return D3D12_BLEND_SRC_COLOR;
        case BlendFactor::InvSrcColor:     return D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactor::DestColor:       return D3D12_BLEND_DEST_COLOR;
        case BlendFactor::InvDestColor:    return D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactor::SrcAlpha:        return D3D12_BLEND_SRC_ALPHA;
        case BlendFactor::InvSrcAlpha:     return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactor::DestAlpha:       return D3D12_BLEND_DEST_ALPHA;
        case BlendFactor::InvDestAlpha:    return D3D12_BLEND_INV_DEST_ALPHA;
        default:
			AUX::Unreachable();
        }
    }

    inline D3D12_BLEND_OP MapBlendOp(BlendOp InOp)
    {
        switch (InOp)
        {
        case BlendOp::Add:            return D3D12_BLEND_OP_ADD;
        case BlendOp::Substract:      return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::Min:            return D3D12_BLEND_OP_MIN;
        case BlendOp::Max:            return D3D12_BLEND_OP_MAX;
        default:
			AUX::Unreachable();
        }
    }

    inline D3D12_CULL_MODE MapRasterizerCullMode(RasterizerCullMode InMode)
    {
        switch (InMode)
        {
        case RasterizerCullMode::None:     return D3D12_CULL_MODE_NONE;
        case RasterizerCullMode::Front:    return D3D12_CULL_MODE_FRONT;
        case RasterizerCullMode::Back:     return D3D12_CULL_MODE_BACK;
        default:
			AUX::Unreachable();
        }
    }

    inline D3D12_FILL_MODE MapRasterizerFillMode(RasterizerFillMode InMode)
    {
        switch (InMode)
        {
        case RasterizerFillMode::WireFrame:    return D3D12_FILL_MODE_WIREFRAME;
        case RasterizerFillMode::Solid:        return D3D12_FILL_MODE_SOLID;
        default:
			AUX::Unreachable();
        }
    }

	inline D3D12_RASTERIZER_DESC MapRasterizerState(RasterizerStateDesc InDesc)
	{
		CD3DX12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT{});
		Desc.FillMode = MapRasterizerFillMode(InDesc.FillMode);
		Desc.CullMode = MapRasterizerCullMode(InDesc.CullMode);
		return Desc;
	}

    inline DXGI_FORMAT MapTextureFormat(GpuTextureFormat InTexFormat)
    {
        switch (InTexFormat)
        {
        case GpuTextureFormat::R8G8B8A8_UNORM:        return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GpuTextureFormat::B8G8R8A8_UNORM:        return DXGI_FORMAT_B8G8R8A8_UNORM;
		case GpuTextureFormat::B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case GpuTextureFormat::R10G10B10A2_UNORM:     return DXGI_FORMAT_R10G10B10A2_UNORM;
        case GpuTextureFormat::R16G16B16A16_UNORM:    return DXGI_FORMAT_R16G16B16A16_UNORM;
        case GpuTextureFormat::R16G16B16A16_UINT:     return DXGI_FORMAT_R16G16B16A16_UINT;
        case GpuTextureFormat::R32G32B32A32_UINT:     return DXGI_FORMAT_R32G32B32A32_UINT;
        case GpuTextureFormat::R16G16B16A16_FLOAT:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GpuTextureFormat::R32G32B32A32_FLOAT:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GpuTextureFormat::R11G11B10_FLOAT:       return DXGI_FORMAT_R11G11B10_FLOAT;
		case GpuTextureFormat::R16_FLOAT:             return DXGI_FORMAT_R16_FLOAT;
        case GpuTextureFormat::R32_FLOAT:             return DXGI_FORMAT_R32_FLOAT;
		default:
			AUX::Unreachable();
        }
    }

    inline D3D12_PRIMITIVE_TOPOLOGY MapPrimitiveType(PrimitiveType InType)
    {
        switch (InType)
        {
        case PrimitiveType::PointList:            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveType::LineList:             return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveType::LineStrip:        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveType::TriangleList:         return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveType::TriangleStrip:    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
			AUX::Unreachable();
        }
    }

	inline D3D12_COMPARISON_FUNC MapComparisonFunc(CompareMode InMode)
	{
		switch (InMode)
		{
		case CompareMode::Less:				return D3D12_COMPARISON_FUNC_LESS;
		case CompareMode::LessEqual:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareMode::Never:			return D3D12_COMPARISON_FUNC_NEVER;
		case CompareMode::Always:			return D3D12_COMPARISON_FUNC_ALWAYS;
		case CompareMode::Equal:			return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareMode::Greater:			return D3D12_COMPARISON_FUNC_GREATER;
		case CompareMode::GreaterEqual:		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareMode::NotEqual:			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		default:
			AUX::Unreachable();
		}
	}

	inline D3D12_TEXTURE_ADDRESS_MODE MapTextureAddressMode(SamplerAddressMode InMode)
	{
		switch (InMode)
		{
		case SamplerAddressMode::Clamp:		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case SamplerAddressMode::Mirror:	return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case SamplerAddressMode::Wrap:		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		default:
			AUX::Unreachable();
		}
	}

}
