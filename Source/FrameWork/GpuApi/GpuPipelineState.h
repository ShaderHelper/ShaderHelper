#pragma once
#include "GpuResourceCommon.h"
#include "GpuShader.h"
#include "GpuBindGroupLayout.h"

namespace FW
{
    struct RasterizerStateDesc
    {
        RasterizerFillMode FillMode;
        RasterizerCullMode CullMode;
    };

    struct PipelineTargetDesc
    {
		GpuTextureFormat TargetFormat;

		bool BlendEnable = false;
		BlendFactor SrcFactor = BlendFactor::SrcAlpha;
		BlendOp ColorOp = BlendOp::Add;
		BlendFactor DestFactor = BlendFactor::InvSrcAlpha;
		BlendFactor SrcAlphaFactor = BlendFactor::One;
		BlendOp AlphaOp = BlendOp::Add;
		BlendFactor DestAlphaFactor = BlendFactor::One;
		BlendMask Mask = BlendMask::All;
    };

	struct GpuComputePipelineStateDesc
	{
		GpuShader* Cs;

		GpuBindGroupLayout* BindGroupLayout0 = nullptr;
		GpuBindGroupLayout* BindGroupLayout1 = nullptr;
		GpuBindGroupLayout* BindGroupLayout2 = nullptr;
		GpuBindGroupLayout* BindGroupLayout3 = nullptr;
	};

    struct GpuRenderPipelineStateDesc
    {
        GpuShader* Vs;
        GpuShader* Ps;
		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;

		GpuBindGroupLayout* BindGroupLayout0 = nullptr;
		GpuBindGroupLayout* BindGroupLayout1 = nullptr;
		GpuBindGroupLayout* BindGroupLayout2 = nullptr;
		GpuBindGroupLayout* BindGroupLayout3 = nullptr;

		RasterizerStateDesc RasterizerState{ RasterizerFillMode::Solid, RasterizerCullMode::None };
		PrimitiveType Primitive = PrimitiveType::TriangleList;
    };

    class GpuRenderPipelineState : public GpuResource
    {
	public:
		GpuRenderPipelineState(GpuRenderPipelineStateDesc InDesc) 
			: GpuResource(GpuResourceType::RenderPipelineState)
			, Desc(MoveTemp(InDesc))
		{}
		const GpuRenderPipelineStateDesc& GetDesc() const { return Desc; }

	private:
		GpuRenderPipelineStateDesc Desc;
    };

	class GpuComputePipelineState : public GpuResource
	{
	public:
		GpuComputePipelineState(GpuComputePipelineStateDesc InDesc) 
			: GpuResource(GpuResourceType::ComputePipelineState)
			, Desc(MoveTemp(InDesc))
		{
		}
		const GpuComputePipelineStateDesc& GetDesc() const { return Desc; }

	private:
		GpuComputePipelineStateDesc Desc;
	};
}

