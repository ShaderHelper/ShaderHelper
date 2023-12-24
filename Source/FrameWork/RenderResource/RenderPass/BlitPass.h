#pragma once
#include "Renderer/RenderGraph.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	struct BlitPassInput
	{
		GpuTexture* InputTex;
		GpuSampler* InputTexSampler;

		GpuTexture* OutputRenderTarget;

		FUintVector2 ViewRect;
	};

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput);
}