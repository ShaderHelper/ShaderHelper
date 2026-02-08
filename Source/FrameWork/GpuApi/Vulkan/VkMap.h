#pragma once
#include "VkCommon.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	inline VkFormat MapTextureFormat(GpuTextureFormat InTexFormat)
	{
		switch (InTexFormat)
		{
		case GpuTextureFormat::R8_UNORM:              return VK_FORMAT_R8_UNORM;
		case GpuTextureFormat::R8G8B8A8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
		case GpuTextureFormat::B8G8R8A8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
		case GpuTextureFormat::B8G8R8A8_UNORM_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
		case GpuTextureFormat::R10G10B10A2_UNORM:     return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case GpuTextureFormat::R16G16B16A16_UNORM:    return VK_FORMAT_R16G16B16A16_UNORM;
		case GpuTextureFormat::R16G16B16A16_UINT:     return VK_FORMAT_R16G16B16A16_UINT;
		case GpuTextureFormat::R32G32B32A32_UINT:     return VK_FORMAT_R32G32B32A32_UINT;
		case GpuTextureFormat::R16G16B16A16_FLOAT:    return VK_FORMAT_R16G16B16A16_SFLOAT;
		case GpuTextureFormat::R32G32B32A32_FLOAT:    return VK_FORMAT_R32G32B32A32_SFLOAT;
		case GpuTextureFormat::R11G11B10_FLOAT:       return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case GpuTextureFormat::R16_FLOAT:             return VK_FORMAT_R16_SFLOAT;
		case GpuTextureFormat::R32_FLOAT:             return VK_FORMAT_R32_SFLOAT;
		default:
			AUX::Unreachable();
		}
	}

	inline VkAttachmentLoadOp MapLoadAction(RenderTargetLoadAction InLoadAction)
	{
		switch (InLoadAction)
		{
		case RenderTargetLoadAction::DontCare:      return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case RenderTargetLoadAction::Load:          return VK_ATTACHMENT_LOAD_OP_LOAD;
		case RenderTargetLoadAction::Clear:         return VK_ATTACHMENT_LOAD_OP_CLEAR;
		default:
			AUX::Unreachable();
		}
	}

	inline VkAttachmentStoreOp MapStoreAction(RenderTargetStoreAction InStoreAction)
	{
		switch (InStoreAction)
		{
		case RenderTargetStoreAction::DontCare:      return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case RenderTargetStoreAction::Store:         return VK_ATTACHMENT_STORE_OP_STORE;
		default:
			AUX::Unreachable();
		}
	}

	inline VkBlendFactor MapBlendFactor(BlendFactor InFactor)
	{
		switch (InFactor)
		{
		case BlendFactor::Zero:            return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One:             return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor:        return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvSrcColor:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DestColor:       return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvDestColor:    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlpha:        return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:     return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DestAlpha:       return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvDestAlpha:    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		default:
			AUX::Unreachable();
		}
	}

	inline VkBlendOp MapBlendOp(BlendOp InOp)
	{
		switch (InOp)
		{
		case BlendOp::Add:            return VK_BLEND_OP_ADD;
		case BlendOp::Substract:      return VK_BLEND_OP_SUBTRACT;
		case BlendOp::Min:            return VK_BLEND_OP_MIN;
		case BlendOp::Max:            return VK_BLEND_OP_MAX;
		default:
			AUX::Unreachable();
		}
	}

	inline VkColorComponentFlags MapColorWriteMask(BlendMask InMask)
	{
		VkColorComponentFlags Flags = 0;
		if (EnumHasAnyFlags(InMask, BlendMask::R)) Flags |= VK_COLOR_COMPONENT_R_BIT;
		if (EnumHasAnyFlags(InMask, BlendMask::G)) Flags |= VK_COLOR_COMPONENT_G_BIT;
		if (EnumHasAnyFlags(InMask, BlendMask::B)) Flags |= VK_COLOR_COMPONENT_B_BIT;
		if (EnumHasAnyFlags(InMask, BlendMask::A)) Flags |= VK_COLOR_COMPONENT_A_BIT;
		return Flags;
	}

	inline VkCullModeFlags MapRasterizerCullMode(RasterizerCullMode InMode)
	{
		switch (InMode)
		{
		case RasterizerCullMode::None:     return VK_CULL_MODE_NONE;
		case RasterizerCullMode::Front:    return VK_CULL_MODE_FRONT_BIT;
		case RasterizerCullMode::Back:     return VK_CULL_MODE_BACK_BIT;
		default:
			AUX::Unreachable();
		}
	}

	inline VkPolygonMode MapRasterizerFillMode(RasterizerFillMode InMode)
	{
		switch (InMode)
		{
		case RasterizerFillMode::WireFrame:    return VK_POLYGON_MODE_LINE;
		case RasterizerFillMode::Solid:        return VK_POLYGON_MODE_FILL;
		default:
			AUX::Unreachable();
		}
	}

	inline VkPrimitiveTopology MapPrimitiveType(PrimitiveType InType)
	{
		switch (InType)
		{
		case PrimitiveType::PointList:            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveType::LineList:             return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveType::LineStrip:            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case PrimitiveType::TriangleList:         return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveType::TriangleStrip:        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:
			AUX::Unreachable();
		}
	}
}
