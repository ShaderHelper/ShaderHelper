#pragma once
#include "GpuApi/GpuRhi.h"

namespace FRAMEWORK
{
	//TODO

	using RenderPassExecution = TFunction<void(GpuRenderPassRecorder*)>;

	enum class RGPassFlag
	{
		Render,
		Compute,
	};

	struct RGPass
	{
		FString Name;
		RGPassFlag Flag;
	};

	struct RGRenderPass : RGPass
	{
		GpuRenderPassDesc Desc;
		RenderPassExecution Execution;
	};

	class RenderGraph
	{
	public:
		RenderGraph();
		RenderGraph(GpuCmdRecorder* InCmdRecorder);
		void AddRenderPass(const FString& PassName, const GpuRenderPassDesc& PassInfo,
			const RenderPassExecution& InExecution);

		void Execute();

	private:
		TArray<TUniquePtr<RGPass>> RGPasses;
		GpuCmdRecorder* CmdRecorder;
	};
}