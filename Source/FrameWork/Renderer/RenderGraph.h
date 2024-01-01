#pragma once
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	//TODO

	using PassExecution = TFunction<void()>;

	struct RGPass
	{
		FString Name;
		GpuRenderPassDesc Info;
		PassExecution Execution;
	};

	class RenderGraph
	{
	public:
		RenderGraph() = default;
		void AddPass(const FString& PassName, const GpuRenderPassDesc& PassInfo,
			const PassExecution& InExecution);

		void Execute();

	private:
		TArray<RGPass> RGPasses;
	};
}