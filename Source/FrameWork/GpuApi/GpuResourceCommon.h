#pragma once

namespace FW
{
	namespace GpuResourceLimit
	{
		inline constexpr int32 MaxRenderTargetNum = 8; //follow dx12:D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT
		inline constexpr int32 MaxBindableBingGroupNum = 4; //Only support 4 BindGroups to adapt some mobile devices.
	}

	enum class GpuResourceState : uint32
	{
		Unknown = 0,
		//Read state
		UniformBuffer = 1u << 0,
		ShaderResourceRead = 1u << 1,
		CopySrc = 1u << 2,

		//Write State
		RenderTargetWrite = 1u << 3,
		CopyDst = 1u << 4,
		UnorderedAccess = 1u << 5,

		ReadMask = ShaderResourceRead | CopySrc,
		WriteMask = RenderTargetWrite | CopyDst | UnorderedAccess,
	};
	ENUM_CLASS_FLAGS(GpuResourceState);

	enum class GpuTextureFormat
	{
		//Unorm
		R8_UNORM,
		R8G8B8A8_UNORM,
		B8G8R8A8_UNORM,
		B8G8R8A8_UNORM_SRGB,
		R10G10B10A2_UNORM,
		R16G16B16A16_UNORM,

		//Uint
		R16G16B16A16_UINT,
		R32G32B32A32_UINT,

		//Float
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT,
		R11G11B10_FLOAT,
		R16_FLOAT,
        R32_FLOAT,
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

	enum class GpuBufferUsage : uint32
	{
		Upload = 1u << 0,
		ReadBack = 1u << 1,

		//Persistent(1 or more frames) if not specified
		Temporary = 1u << 2, // at most 1 frame.

		Uniform = 1u << 3,
		Structured = 1u << 4,
		RWStructured = 1u << 5,
		Raw = 1u << 6,
		RWRaw = 1u << 7,

		StaticMask = RWStructured | Structured | Raw | RWRaw, //Only Gpu r/w
		DynamicMask = Upload | ReadBack | Uniform, //Cpu can r/w
	};
	ENUM_CLASS_FLAGS(GpuBufferUsage);

	enum class GpuResourceType
	{
		Buffer,
		Texture,
		BindGroup,
		BindGroupLayout,
		RenderPipelineState,
		ComputePipelineState,
		Shader,
		Sampler,
	};

	enum class GpuResourceMapMode
	{
		Read_Only,
		Write_Only,
	};

	enum class PrimitiveType
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	enum class ShaderType
	{
		VertexShader,
		PixelShader,
		ComputeShader,
		Num
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

	enum class SamplerFilter
	{
		Point,
		Bilinear,
		Trilinear,
	};

	enum class SamplerAddressMode
	{
		Clamp,
		Wrap,
		Mirror,
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

    class GpuResource : public FThreadSafeRefCountedObject 
	{
	public:
		GpuResource(GpuResourceType InType) : Type(InType) {}
		GpuResourceType GetType() const { return Type; }

	private:
		GpuResourceType Type;
	};

	class GpuTrackedResource : public GpuResource
	{
	public:
		GpuTrackedResource(GpuResourceType InType, GpuResourceState InState);
		GpuResourceState State;
	};

	FRAMEWORK_API uint32 GetTextureFormatByteSize(GpuTextureFormat InFormat);

	FRAMEWORK_API GpuResourceState GetBufferState(GpuBufferUsage Usage);

	FRAMEWORK_API GpuResourceState GetTextureState(GpuTextureUsage Usage);
}

