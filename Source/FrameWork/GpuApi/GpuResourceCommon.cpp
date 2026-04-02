#include "CommonHeader.h"
#include "GpuResourceCommon.h"
#include "GpuApiValidation.h"

namespace FW
{
	uint32 GetFormatByteSize(GpuFormat InFormat)
	{
		switch (InFormat)
		{
		case GpuFormat::R8_UNORM:
			return 1;
		case GpuFormat::R16_UINT:
		case GpuFormat::R16_FLOAT:
			return 2;
		case GpuFormat::R8G8B8A8_UNORM:
		case GpuFormat::B8G8R8A8_UNORM:
		case GpuFormat::B8G8R8A8_UNORM_SRGB:
		case GpuFormat::R10G10B10A2_UNORM:
		case GpuFormat::R11G11B10_FLOAT:
		case GpuFormat::R32_UINT:
		case GpuFormat::R32_FLOAT:
		case GpuFormat::D32_FLOAT:
			return 4;
		case GpuFormat::R32G32_UINT:
		case GpuFormat::R32G32_FLOAT:
		case GpuFormat::R16G16B16A16_UNORM:
		case GpuFormat::R16G16B16A16_UINT:
		case GpuFormat::R16G16B16A16_FLOAT:
			return 8;
		case GpuFormat::R32G32B32_UINT:
		case GpuFormat::R32G32B32_FLOAT:
			return 12;
		case GpuFormat::R32G32B32A32_UINT:
		case GpuFormat::R32G32B32A32_FLOAT:
			return 16;
		default:
			AUX::Unreachable();
		}
	}

	bool IsDepthFormat(GpuFormat InFormat)
	{
		switch (InFormat)
		{
		case GpuFormat::D32_FLOAT:
			return true;
		default:
			return false;
		}
	}

	GpuResourceState GetBufferState(GpuBufferUsage Usage)
	{
		GpuResourceState Result = GpuResourceState::Unknown;
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::RWStructured | GpuBufferUsage::RWRaw))
		{
			Result |= GpuResourceState::UnorderedAccess;
		}
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Structured | GpuBufferUsage::Raw))
		{
			Result |= GpuResourceState::ShaderResourceRead;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Uniform))
		{
			Result |= GpuResourceState::UniformBuffer;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Vertex))
		{
			Result |= GpuResourceState::VertexBufferRead;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Index))
		{
			Result |= GpuResourceState::IndexBufferRead;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::ReadBack))
		{
			Result |= GpuResourceState::CopyDst;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Upload))
		{
			Result |= GpuResourceState::CopySrc;
		}

		return Result;
	}

	GpuResourceState GetTextureState(GpuTextureUsage Usage)
	{
		GpuResourceState Result = GpuResourceState::Unknown;
		if (EnumHasAllFlags(Usage, GpuTextureUsage::RenderTarget)) {
			Result |= GpuResourceState::RenderTargetWrite;
		}
		if (EnumHasAllFlags(Usage, GpuTextureUsage::DepthStencil)) {
			Result |= GpuResourceState::DepthStencilWrite;
		}
		if (EnumHasAllFlags(Usage, GpuTextureUsage::UnorderedAccess)) {
			Result |= GpuResourceState::UnorderedAccess;
		}
		if (EnumHasAllFlags(Usage, GpuTextureUsage::ShaderResource)) {
			Result |= GpuResourceState::ShaderResourceRead;
		}
		return Result;
	}
}
