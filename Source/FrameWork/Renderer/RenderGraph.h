#pragma once
#include "GpuApi/GpuRhi.h"

namespace FW
{
	//TODO

	using RenderPassExecution = TFunction<void(GpuRenderPassRecorder*)>;

	struct RGRenderPass
	{
        FString Name;
		GpuRenderPassDesc Desc;
		RenderPassExecution Execution;
	};

	class FRAMEWORK_API RenderGraph : FNoncopyable
	{
	public:
		RenderGraph();
		RenderGraph(GpuCmdRecorder* InCmdRecorder);
		void AddRenderPass(const FString& PassName, const GpuRenderPassDesc& PassInfo,
			const RenderPassExecution& InExecution);

		void Execute();

	private:
		TArray<RGRenderPass> RGRenderPasses;
		GpuCmdRecorder* CmdRecorder;
	};
}
