#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
    inline D3D12_RASTERIZER_DESC MapRasterizerState(RasterizerStateDesc InDesc)
    {
        CD3DX12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT{});
        Desc.FillMode = MapRasterizerFillMode(InDesc.FillMode);
        Desc.CullMode = MapRasterizerCullMode(InDesc.CullMode);
        return Desc;
    }

    inline D3D12_BLEND_DESC MapBlendState(BlendStateDesc InDesc)
    {
        CD3DX12_BLEND_DESC Desc = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT{});
        const uint32 BlendRtNum = InDesc.RtDescs.Num();
        for (uint32 i = 0; i < BlendRtNum; i++)
        {
            Desc.RenderTarget[i].BlendEnable = InDesc.RtDescs[i].BlendEnable;
            Desc.RenderTarget[i].SrcBlend = MapBlendFactor(InDesc.RtDescs[i].SrcFactor);
            Desc.RenderTarget[i].SrcBlendAlpha = MapBlendFactor(InDesc.RtDescs[i].SrcAlphaFactor);
            Desc.RenderTarget[i].DestBlend = MapBlendFactor(InDesc.RtDescs[i].DestFactor);
            Desc.RenderTarget[i].DestBlendAlpha = MapBlendFactor(InDesc.RtDescs[i].DestAlphaFactor);
            Desc.RenderTarget[i].BlendOp = MapBlendOp(InDesc.RtDescs[i].ColorOp);
            Desc.RenderTarget[i].BlendOpAlpha = MapBlendOp(InDesc.RtDescs[i].AlphaOp);
            Desc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE(uint32(InDesc.RtDescs[i].Mask));
        }
        return Desc;
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
            SH_LOG(LogDx12, Fatal, TEXT("Invalid BlendFactor."));
            return D3D12_BLEND_ZERO;
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
            SH_LOG(LogDx12, Fatal, TEXT("Invalid BlendOp."));
            return D3D12_BLEND_OP_ADD;
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
            SH_LOG(LogDx12, Fatal, TEXT("Invalid CullMode."));
            return D3D12_CULL_MODE_NONE;
        }
    }

    inline D3D12_FILL_MODE MapRasterizerFillMode(RasterizerFillMode InMode)
    {
        switch (InMode)
        {
        case RasterizerFillMode::WireFrame:    return D3D12_FILL_MODE_WIREFRAME;
        case RasterizerFillMode::Solid:        return D3D12_FILL_MODE_SOLID;
        default:
            SH_LOG(LogDx12, Fatal, TEXT("Invalid CullMode."));
            return D3D12_FILL_MODE_SOLID;
        }
    }

    inline DXGI_FORMAT MapTextureFormat(GpuTextureFormat InTexFormat)
    {
        switch (InTexFormat)
        {
        case GpuTextureFormat::R8G8B8A8_UNORM:        return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GpuTextureFormat::B8G8R8A8_UNORM:        return DXGI_FORMAT_B8G8R8A8_UNORM;
        case GpuTextureFormat::R10G10B10A2_UNORM:     return DXGI_FORMAT_R10G10B10A2_UNORM;
        case GpuTextureFormat::R16G16B16A16_UNORM:    return DXGI_FORMAT_R16G16B16A16_UNORM;
        case GpuTextureFormat::R16G16B16A16_UINT:     return DXGI_FORMAT_R16G16B16A16_UINT;
        case GpuTextureFormat::R32G32B32A32_UINT:     return DXGI_FORMAT_R32G32B32A32_UINT;
        case GpuTextureFormat::R16G16B16A16_FLOAT:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GpuTextureFormat::R32G32B32A32_FLOAT:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GpuTextureFormat::R11G11B10_FLOAT:       return DXGI_FORMAT_R11G11B10_FLOAT;
        default:
            SH_LOG(LogDx12, Fatal, TEXT("Invalid GpuTextureFormat."));
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }

    inline D3D12_PRIMITIVE_TOPOLOGY MapPrimitiveType(PrimitiveType InType)
    {
        switch (InType)
        {
        case PrimitiveType::Point:            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveType::Line:             return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveType::LineStrip:        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveType::Triangle:         return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case PrimitiveType::TriangleStrip:    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            SH_LOG(LogDx12, Fatal, TEXT("Invalid PrimitiveType."));
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        }
    }
}
