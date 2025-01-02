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

    struct GpuRenderPipelineStateDesc
    {
        GpuShader* Vs;
        GpuShader* Ps;
		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;

		struct
		{
			GpuBindGroupLayout* BindGroupLayout0 = nullptr;
			GpuBindGroupLayout* BindGroupLayout1 = nullptr;
			GpuBindGroupLayout* BindGroupLayout2 = nullptr;
			GpuBindGroupLayout* BindGroupLayout3 = nullptr;
		};

		RasterizerStateDesc RasterizerState{ RasterizerFillMode::Solid, RasterizerCullMode::None };
		PrimitiveType Primitive = PrimitiveType::TriangleList;
    };

    class GpuPipelineState : public GpuResource
    {
	public:
		GpuPipelineState() : GpuResource(GpuResourceType::PipelineState)
		{}
    };
}

