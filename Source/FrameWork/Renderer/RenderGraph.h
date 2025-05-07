#pragma once
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Shader/Shader.h"

namespace FW
{
	//TODO

	using RenderPassExecution = TFunction<void(GpuRenderPassRecorder*, BindingContext&)>;

	struct RGRenderPass
	{
        FString Name;
		GpuRenderPassDesc Desc;
		BindingContext Bindings;
		RenderPassExecution Execution;

		TMap<GpuTexture*, GpuResourceState> PassTexStates;
		TMap<GpuBuffer*, GpuResourceState> PassBufferStates;
	};

	class FRAMEWORK_API RenderGraph : FNoncopyable
	{
	public:
		RenderGraph();
		RenderGraph(GpuCmdRecorder* InCmdRecorder);
		void AddRenderPass(const FString& PassName, const GpuRenderPassDesc& PassInfo, const BindingContext& Bindings,
			const RenderPassExecution& InExecution);

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
