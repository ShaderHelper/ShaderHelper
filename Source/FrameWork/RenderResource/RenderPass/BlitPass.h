#pragma once
#include "Renderer/RenderGraph.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	struct BlitPassInput
	{
		TRefCountPtr<GpuTexture> InputTex;
		TRefCountPtr<GpuSampler> InputTexSampler;

		TRefCountPtr<GpuTexture> OutputRenderTarget;

		TOptional<GpuScissorRectDesc> Scissor;
		RenderTargetLoadAction LoadAction = RenderTargetLoadAction::Load;
	};

	FRAMEWORK_API void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput);
}
