#pragma once
#include "CommonHeader.h"

namespace FRAMEWORK
{
    inline constexpr int MaxRenderTargetNum = 8; //follow dx12:D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT

    enum class GpuTextureFormat
    {
        //Unorm
        R8G8B8A8_UNORM,
        B8G8R8A8_UNORM,
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

	enum class GpuBufferUsage
	{
		Vertex,
		Index,
		Uniform,
	};

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

    inline uint32 GetTextureFormatByteSize(GpuTextureFormat InFormat)
    {
        switch (InFormat)
        {
        case GpuTextureFormat::R8G8B8A8_UNORM:
        case GpuTextureFormat::B8G8R8A8_UNORM:
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

