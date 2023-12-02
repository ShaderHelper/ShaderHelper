#pragma once
#include "GpuResourceCommon.h"
#include "GpuShader.h"
#include "GpuBindGroupLayout.h"

namespace FRAMEWORK
{
    struct RasterizerStateDesc
    {
        RasterizerFillMode FillMode;
        RasterizerCullMode CullMode;
    };

    struct BlendRenderTargetDesc
    {
        bool BlendEnable;
        BlendFactor SrcFactor;
		BlendOp ColorOp;
        BlendFactor DestFactor;
        BlendFactor SrcAlphaFactor;
		BlendOp AlphaOp;
        BlendFactor DestAlphaFactor;

		BlendMask Mask = BlendMask::All;
    };

    struct BlendStateDesc
    {
		TArray<BlendRenderTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> RtDescs;
    };

    struct PipelineStateDesc
    {
        GpuShader* Vs;
        GpuShader* Ps;
        RasterizerStateDesc RasterizerState;
        BlendStateDesc BlendState;
		TArray<GpuTextureFormat, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> RtFormats;

		GpuBindGroupLayout* BindGroupLayout0 = nullptr;
		GpuBindGroupLayout* BindGroupLayout1 = nullptr;
		GpuBindGroupLayout* BindGroupLayout2 = nullptr;
		GpuBindGroupLayout* BindGroupLayout3 = nullptr;
    };

    class GpuPipelineState : public GpuResource
    {
	public:
		GpuPipelineState() : GpuResource(GpuResourceType::PipelineState)
		{}
    };
}

