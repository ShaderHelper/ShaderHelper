#pragma once
#include "GpuResourceCommon.h"
#include "GpuPipelineState.h"
#include "GpuTexture.h"
#include "GpuShader.h"
#include "GpuBuffer.h"
#include "GpuRenderPass.h"
#include "GpuBindGroup.h"
#include "GpuBindGroupLayout.h"

namespace FRAMEWORK
{
	namespace GpuResourceHelper
	{
		const inline BlendStateDesc GDefaultBlendStateDesc{
			BlendStateDesc::DescStorageType{ 
				BlendRenderTargetDesc{ false, BlendFactor::SrcAlpha, BlendFactor::InvSrcAlpha, BlendFactor::One, BlendFactor::One, BlendOp::Add, BlendOp::Add}
			}
		};
	}

}
