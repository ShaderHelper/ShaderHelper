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

	enum class PrimitiveType
	{
		Point,
		Line,
		LineStrip,
		Triangle,
		TriangleStrip,
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
		None = 0, //All Color channels are disabled.
		R = 1u << 0,
		G = 1u << 1,
		B = 1u << 2,
		A = 1u << 3,
        All = R | G | B | A,
	};
	ENUM_CLASS_FLAGS(BlendMask);

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

    enum class RenderTargetLoadAction
    {
        DontCare,
        Load,
        Clear,
    };

    enum class RenderTargetStoreAction
    {
        DontCare,
        Store,
    };


	class GpuResource : public FThreadSafeRefCountedObject {};

	class GpuBuffer : public GpuResource {};

	class GpuTexture : public GpuResource {
	public:
		GpuTexture(uint32 InWidth, uint32 InHeight, GpuTextureFormat InFormat) 
			: Width(InWidth), Height(InHeight), Format(InFormat)
		{}
		GpuTextureFormat GetFormat() const { return Format; }
		uint32 GetWidth() const { return Width; }
		uint32 GetHeight() const { return Height; }

	private:
		uint32 Width, Height;
		GpuTextureFormat Format;
	};

    class GpuRenderTargetInfo
    {
    public:
        GpuRenderTargetInfo(GpuTexture* InTexture,
                        RenderTargetLoadAction InLoadAction = RenderTargetLoadAction::DontCare,
                        RenderTargetStoreAction InStoreAction = RenderTargetStoreAction::DontCare)
            : Texture(InTexture)
            , LoadAction(InLoadAction)
            , StoreAction(InStoreAction)
        {}
    public:
        GpuTexture* GetRenderTarget() const { return Texture; }
        
    public:
        RenderTargetLoadAction LoadAction;
        RenderTargetStoreAction StoreAction;
        
    private:
        GpuTexture* Texture;
    };

	class GpuShader : public GpuResource {};

	class RenderPipelineState : public GpuResource {};

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

	struct RasterizerStateDesc
	{
		RasterizerStateDesc(RasterizerFillMode InFillMode, RasterizerCullMode InCullMode)
			: FillMode(InFillMode)
			, CullMode(InCullMode)
		{}

		RasterizerFillMode FillMode;
		RasterizerCullMode CullMode;
	};

	struct BlendRenderTargetDesc
	{
		BlendRenderTargetDesc(
			bool InBlendEnable,
			BlendFactor InSrcFactor, BlendFactor InDestFactor,
			BlendFactor InSrcAlphaFactor, BlendFactor InDestAlphaFactor,
			BlendOp InColorOp, BlendOp InAlphaOp, BlendMask InMask = BlendMask::All)
			: BlendEnable(InBlendEnable)
			, SrcFactor(InSrcFactor)
			, DestFactor(InDestFactor)
			, SrcAlphaFactor(InSrcAlphaFactor)
			, DestAlphaFactor(InDestAlphaFactor)
			, Mask(InMask)
			, ColorOp(InColorOp)
			, AlphaOp(InAlphaOp)
		{
		}
		bool BlendEnable;
		BlendFactor SrcFactor;
		BlendFactor DestFactor;
		BlendFactor SrcAlphaFactor;
		BlendFactor DestAlphaFactor;
		BlendMask Mask;
		BlendOp ColorOp;
		BlendOp AlphaOp;
	};

    inline constexpr int MaxRenderTargetNum = 8; //follow dx12:D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT

	struct BlendStateDesc
	{
		using DescStorageType = TArray<BlendRenderTargetDesc, TFixedAllocator<MaxRenderTargetNum>>;
		BlendStateDesc() = default;
		BlendStateDesc(DescStorageType InDescStorage) : RtDescs(MoveTemp(InDescStorage)) {}
		DescStorageType RtDescs;
	};

	struct PipelineStateDesc
	{
        using RtFormatStorageType = TArray<GpuTextureFormat, TFixedAllocator<MaxRenderTargetNum>>;
		PipelineStateDesc(
			TRefCountPtr<GpuShader> InVs, TRefCountPtr<GpuShader> InPs,
			RasterizerStateDesc InRasterizerState, BlendStateDesc InBlendState,
            RtFormatStorageType InRtFormats)
			: Vs(MoveTemp(InVs))
			, Ps(MoveTemp(InPs))
			, RasterizerState(MoveTemp(InRasterizerState))
			, BlendState(MoveTemp(InBlendState))
			, RtFormats(MoveTemp(InRtFormats))
		{
		}
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;
		RasterizerStateDesc RasterizerState;
		BlendStateDesc BlendState;
        RtFormatStorageType RtFormats;
	};

	struct GpuViewPortDesc
	{
		GpuViewPortDesc(uint32 InWidth, uint32 InHeight, float InZMin = 0.0f, float InZMax = 1.0f, float InTopLeftX = 0.0f, float InTopLeftY = 0.0f)
			: TopLeftX(InTopLeftX)
			, TopLeftY(InTopLeftY)
			, Width(InWidth)
			, Height(InHeight)
			, ZMin(InZMin)
			, ZMax(InZMax)
		{
		}
		float TopLeftX, TopLeftY;
		uint32 Width, Height;
		float ZMin, ZMax;
	};

	inline uint32 GetTextureFormatByteSize(GpuTextureFormat InFormat)
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

    struct GpuRenderPassDesc
    {
        TArray<GpuRenderTargetInfo, TFixedAllocator<MaxRenderTargetNum>> ColorRenderTargets;
    };

	namespace GpuResourceHelper
	{
		const inline BlendStateDesc GDefaultBlendStateDesc{
			BlendStateDesc::DescStorageType{ 
				BlendRenderTargetDesc{ false, BlendFactor::SrcAlpha, BlendFactor::InvSrcAlpha, BlendFactor::One, BlendFactor::One, BlendOp::Add, BlendOp::Add}
			}
		};
	}
}
