#include "CommonHeader.h"
#include "GpuResourceCommon.h"
#include "GpuApiValidation.h"

namespace FW
{
	GpuTrackedResource::GpuTrackedResource(GpuResourceType InType, GpuResourceState InState)
		: GpuResource(InType)
		, State(InState)
	{
		#if PLATFORM_WINDOWS
		checkf(State != GpuResourceState::Unknown && ValidateGpuResourceState(State), TEXT("The backend can not automatically give a valid state, so explicitly specify it "));
		#endif
	}

	uint32 GetTextureFormatByteSize(GpuTextureFormat InFormat)
	{
		switch (InFormat)
		{
		case GpuTextureFormat::R8_UNORM:
			return 1;
		case GpuTextureFormat::R16_FLOAT:
			return 2;
		case GpuTextureFormat::R8G8B8A8_UNORM:
		case GpuTextureFormat::B8G8R8A8_UNORM:
		case GpuTextureFormat::B8G8R8A8_UNORM_SRGB:
		case GpuTextureFormat::R10G10B10A2_UNORM:
		case GpuTextureFormat::R11G11B10_FLOAT:
		case GpuTextureFormat::R32_FLOAT:
			return 4;
		case GpuTextureFormat::R16G16B16A16_UNORM:
		case GpuTextureFormat::R16G16B16A16_UINT:
		case GpuTextureFormat::R16G16B16A16_FLOAT:
			return 8;
		case GpuTextureFormat::R32G32B32A32_UINT:
		case GpuTextureFormat::R32G32B32A32_FLOAT:
			return 16;
		default:
			AUX::Unreachable();
		}
	}

	GpuResourceState GetBufferState(GpuBufferUsage Usage)
	{
		GpuResourceState Result = GpuResourceState::Unknown;
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::RWStorage | GpuBufferUsage::RWRaw))
		{
			Result |= GpuResourceState::UnorderedAccess;
		}
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Storage | GpuBufferUsage::Raw))
		{
			Result |= GpuResourceState::ShaderResourceRead;
		}
		if (EnumHasAllFlags(Usage, GpuBufferUsage::Uniform))
		{
			Result |= GpuResourceState::UniformBuffer;
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
		if (EnumHasAllFlags(Usage, GpuTextureUsage::ShaderResource)) {
			Result |= GpuResourceState::ShaderResourceRead;
		}
		return Result;
	}
}