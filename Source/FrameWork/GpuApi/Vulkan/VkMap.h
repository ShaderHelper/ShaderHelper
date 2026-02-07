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
}
