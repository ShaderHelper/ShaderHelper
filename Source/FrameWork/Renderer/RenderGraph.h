#pragma once
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Shader/Shader.h"

namespace FW
{
	//TODO

	using RenderPassExecution = TFunction<void(GpuRenderPassRecorder*)>;

	struct FRAMEWORK_API RGRenderPass
	{
        FString Name;
		GpuRenderPassDesc Desc;
		RenderPassExecution Execution;

		TMap<GpuResource*, GpuResourceState> PassResourceStates;

		RGRenderPass& Read(GpuResource* InResource);
		RGRenderPass& Write(GpuResource* InResource);
	};

	class FRAMEWORK_API RenderGraph : FNoncopyable
	{
	public:
		RenderGraph();
		RenderGraph(GpuCmdRecorder* InCmdRecorder);
		RGRenderPass& AddRenderPass(const FString& PassName, GpuRenderPassDesc PassInfo, const RenderPassExecution& InExecution);

		void Execute();
	private:
		void SetupPass(RGRenderPass& InOutPass);
		void CreatePassBarriers(const RGRenderPass& InPass);
		void ExecutePass(RGRenderPass& InPass);

	private:
		TArray<RGRenderPass> RGRenderPasses;
		GpuCmdRecorder* CmdRecorder;
	};
}
