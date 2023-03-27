#pragma once
#include "CommonHeader.h"
#include <array>
namespace FRAMEWORK
{
	class GpuResource : public FThreadSafeRefCountedObject {};

	class GpuBuffer : public GpuResource {};

	class GpuTexture : public GpuResource {};

	class GpuShader : public GpuResource {};

	class RenderPipelineState : public GpuResource {};
	
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
		RenderTarget = 1u << 0,
		ShaderResource = 1u << 1,
		Shared = 1u << 2,
	};
	ENUM_CLASS_FLAGS(GpuTextureUsage);

	enum class GpuResourceMapMode
	{
		Read_Only,
		Write_Only,
	};

	struct GpuTextureDesc
	{
		GpuTextureDesc(
			uint32 InWidth,
			uint32 InHeight,
			GpuTextureFormat InFormat,
			GpuTextureUsage InUsage = GpuTextureUsage::None,
			Vector4f InClearValues = Vector4f{ 0 },
			uint32 InDepth = 1,
			uint32 InNumMips = 1
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

		TArray<uint8> InitialData;
	};

	enum class ShaderType
	{
		VertexShader,
		PixelShader
	};

	enum class RasterizerCullMode
	{
		None,
		Front,
		Back,
	};

	enum class RasterizerFillMode
	{
		WireFrame,
		Solid,
	};

	struct RasterizerStateDesc
	{
		RasterizerStateDesc(RasterizerFillMode InFillMode, RasterizerCullMode InCullMode)
			: FillMode(InFillMode)
			, CullMode(InCullMode)
		{}

		RasterizerFillMode FillMode;
		RasterizerCullMode CullMode;
	};

	enum class BlendFactor
	{
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		DestColor,
		InvDestColor,
		SrcAlpha,
		InvSrcAlpha,
		DestAlpha,
		InvDestAlpha,
	};

	enum class BlendOp
	{
		Add,
		Substract,
		Min,
		Max,
	};

	enum class BlendMask : uint32
	{
		None = 0,
		R = 1u << 0,
		G = 1u << 1,
		B = 1u << 2,
		A = 1u << 3,
	};
	ENUM_CLASS_FLAGS(BlendMask);

	struct BlendRenderTargetDesc
	{
		bool BlendEnable;
		BlendFactor SrcFactor;
		BlendFactor DestFactor;
		BlendFactor SrcAlphaFactor;
		BlendFactor DestAlphaFactor;
		BlendMask Mask;
		BlendOp ColorOp;
		BlendOp AlphaOp;
	};

	struct BlendStateDesc
	{
		TArray<BlendRenderTargetDesc, TInlineAllocator<8>> RtDesc;
	};

	enum class StencilOp
	{
		Zero,
		Replace,
		Keep,
		Invert,
		Increase,
		Decrease,
	};

	enum class CompareMode
	{
		Less,
		Equal,
		LessEqual,
		NotEqual,
		Always,
		Greater,
		GreaterEqual,
		Never,
	};
	
	struct PipelineStateDesc
	{
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;
		RasterizerStateDesc RasterizerState;
		BlendStateDesc BlendState;
		GpuTextureFormat RtFormat;
	};
	
	inline uint32 GetFormatByteSize(GpuTextureFormat InFormat)
	{
		switch (InFormat)
		{
		case GpuTextureFormat::R8G8B8A8_UNORM:
		case GpuTextureFormat::R10G10B10A2_UNORM:
		case GpuTextureFormat::R11G11B10_FLOAT:
			return 4;
		case GpuTextureFormat::R16G16B16A16_UNORM:
		case GpuTextureFormat::R16G16B16A16_UINT:
		case GpuTextureFormat::R16G16B16A16_FLOAT:
			return 8;
		case GpuTextureFormat::R32G32B32A32_UINT:
		case GpuTextureFormat::R32G32B32A32_FLOAT:
			return 16;
		default:
			checkf(false, TEXT("Ivalid Texture Format."));
			return 0;
		}
	}
}