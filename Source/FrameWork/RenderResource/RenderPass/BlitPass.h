#pragma once
#include "Renderer/RenderGraph.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	struct BlitPassInput
	{
		std::set<FString> VariantDefinitions;
		TRefCountPtr<GpuTextureView> InputView;
		TRefCountPtr<GpuSampler> InputTexSampler;

		TRefCountPtr<GpuTextureView> OutputView;
		
		TOptional<GpuScissorRectDesc> Scissor;
		TOptional<GpuViewPortDesc> Viewport;
		RenderTargetLoadAction LoadAction = RenderTargetLoadAction::DontCare;
		int32 MipLevel = -1;
	};

	FRAMEWORK_API void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput);
}
