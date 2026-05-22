#pragma once
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Shader/Shader.h"

namespace FW
{
	//TODO

	using RenderPassExecution = TFunction<void(GpuRenderPassRecorder*)>;
	using ComputePassExecution = TFunction<void(GpuComputePassRecorder*)>;

	struct FRAMEWORK_API RGPassBase
	{
		virtual ~RGPassBase() = default;

		FString Name;
		TMap<GpuResource*, GpuResourceState> PassResourceStates;

		virtual void Execute(GpuCmdRecorder* CmdRecorder) = 0;

	protected:
		void AddResourceState(GpuResource* InResource, GpuResourceState InState);
	};

	struct FRAMEWORK_API RGRenderPass : RGPassBase
	{
		GpuRenderPassDesc Desc;
		RenderPassExecution Execution;

		RGRenderPass& Read(GpuResource* InResource);
		RGRenderPass& Write(GpuResource* InResource);

		void Execute(GpuCmdRecorder* CmdRecorder) override;
	};

	struct FRAMEWORK_API RGComputePass : RGPassBase
	{
		ComputePassExecution Execution;
		TOptional<GpuPassTimestampWrites> TimestampWrites;

		RGComputePass& Read(GpuResource* InResource);
		RGComputePass& Write(GpuResource* InResource);

		void Execute(GpuCmdRecorder* CmdRecorder) override;
	};

	class FRAMEWORK_API RenderGraph : FNoncopyable
	{
	public:
		RenderGraph();
		RenderGraph(GpuCmdRecorder* InCmdRecorder);
		RGRenderPass& AddRenderPass(const FString& PassName, GpuRenderPassDesc PassInfo, const RenderPassExecution& InExecution);
		RGComputePass& AddComputePass(const FString& PassName, const ComputePassExecution& InExecution, TOptional<GpuPassTimestampWrites> TimestampWrites = {});

		void Execute();
	private:
		void SetupRenderPass(RGRenderPass& InOutPass);
		void CreatePassBarriers(const RGPassBase& InPass);

	private:
		TArray<TUniquePtr<RGPassBase>> RGPasses;
		GpuCmdRecorder* CmdRecorder;
	};
}
