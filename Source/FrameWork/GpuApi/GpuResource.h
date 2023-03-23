#pragma once
#include "CommonHeader.h"
#include <array>
namespace FRAMEWORK
{
	enum class GpuTextureFormat
	{
		//Unorm
		R8G8B8A8_UNORM,
		R10G10B10A2_UNORM,
		R16G16B16A16_UNORM,
		
		//Uint
		R16G16B16A16_UINT,
		R32G32B32A32_UINT,

		//Float
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT,
		R11G11B10_FLOAT,
		NUM,
	};

	enum class GpuTextureUsage : uint32
	{
		None = 0,
		RenderTarget = 1u << 1,
		ShaderResource = 1u << 2,
		Shared = 1u << 3,
	};
	ENUM_CLASS_FLAGS(GpuTextureUsage);

	enum class TextureMapMode
	{
		READ_ONLY,
		WRITE_ONLY,
	};
	
	struct GpuTextureDesc
	{
		GpuTextureDesc(
			uint32 InWidth,
			uint32 InHeight,
			GpuTextureFormat InFormat,
			uint32 InDepth = 1,
			uint32 InNumMips = 0,
			GpuTextureUsage InUsage = GpuTextureUsage::None,
			Vector4f InClearValues = Vector4f{0}
		)
			: Width(InWidth), Height(InHeight), Depth(InDepth)
			, NumMips(InNumMips)
			, Format(InFormat)
			, Usage(InUsage)
			, ClearValues(MoveTemp(InClearValues))
		{

		}
		uint32 Width;
		uint32 Height;
		uint32 Depth;
		uint32 NumMips;
		GpuTextureFormat Format;
		GpuTextureUsage Usage;
		Vector4f ClearValues;
	};

	class GpuResource : public FThreadSafeRefCountedObject {
	public:
		virtual ~GpuResource() = default;
	};
	
	class GpuTextureResource : public GpuResource {};

	class GpuShader : public GpuResource {};
}