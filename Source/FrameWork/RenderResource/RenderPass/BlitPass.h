#pragma once
#include "Renderer/RenderGraph.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	struct BlitPassInput
	{
		TRefCountPtr<GpuTexture> InputTex;
		TRefCountPtr<GpuSampler> InputTexSampler;

		TRefCountPtr<GpuTexture> OutputRenderTarget;
	};

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput);
}