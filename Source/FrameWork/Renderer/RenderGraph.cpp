#include "CommonHeader.h"
#include "RenderGraph.h"

namespace FW
{

	RenderGraph::RenderGraph()
	{
		CmdRecorder = GGpuRhi->BeginRecording();
	}

	RenderGraph::RenderGraph(GpuCmdRecorder* InCmdRecorder) : CmdRecorder(InCmdRecorder)
	{

	}

	void RenderGraph::SetupPass(RGRenderPass& InOutPass)
	{
		uint32 PassRtNum = InOutPass.Desc.ColorRenderTargets.Num();
		for (uint32 i = 0; i < PassRtNum; i++)
		{
			const GpuRenderTargetInfo& RtInfo = InOutPass.Desc.ColorRenderTargets[i];
			InOutPass.PassTexStates.Add(RtInfo.Texture, GpuResourceState::RenderTargetWrite);
		}

		InOutPass.Bindings.EnumerateBinding([&](const ResourceBinding& ResourceBindingEntry, const LayoutBinding& LayoutBindingEntry) {
			if (LayoutBindingEntry.Type == BindingType::Texture)
			{
				InOutPass.PassTexStates.Add(static_cast<GpuTexture*>(ResourceBindingEntry.Resource), GpuResourceState::ShaderResourceRead);
			}
		});
	}

	void RenderGraph::CreatePassBarriers(const RGRenderPass& InPass)
	{
		for (auto [Tex, PassTexState] : InPass.PassTexStates)
		{
			GpuResourceState TexState = Tex->State;
			if (TexState != PassTexState)
			{
				//TODO:Batch
				CmdRecorder->Barrier(Tex, PassTexState);
			}
		}
	}

	void RenderGraph::ExecutePass(RGRenderPass& InPass)
	{
		auto PassRecorder = CmdRecorder->BeginRenderPass(InPass.Desc, InPass.Name);
		{
			InPass.Execution(PassRecorder, InPass.Bindings);
		}
		CmdRecorder->EndRenderPass(PassRecorder);
	}

	void RenderGraph::AddRenderPass(const FString& PassName, const GpuRenderPassDesc& PassInfo, const BindingContext& Bindings, const RenderPassExecution& InExecution)
	{
		RGRenderPass NewPass{ PassName, PassInfo, Bindings, InExecution };
		SetupPass(NewPass);
        RGRenderPasses.Add(MoveTemp(NewPass));
	}

	void RenderGraph::Execute()
	{
		for (auto& RenderPass: RGRenderPasses)
		{
			CreatePassBarriers(RenderPass);
			ExecutePass(RenderPass);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});
	}

}
