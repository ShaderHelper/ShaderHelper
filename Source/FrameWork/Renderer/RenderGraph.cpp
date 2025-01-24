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

	void RenderGraph::AddRenderPass(const FString& PassName, const GpuRenderPassDesc& PassInfo, const RenderPassExecution& InExecution)
	{
		auto Pass = MakeUnique<RGRenderPass>(
			RGRenderPass{ {PassName, RGPassFlag::Render}, PassInfo, InExecution}
		);
		RGPasses.Add(MoveTemp(Pass));
	}

	void RenderGraph::Execute()
	{
		for (const auto& Pass: RGPasses)
		{
			if (Pass->Flag == RGPassFlag::Render)
			{
				RGRenderPass* RenderPass = static_cast<RGRenderPass*>(Pass.Get());
				auto PassRecorder = CmdRecorder->BeginRenderPass(RenderPass->Desc, RenderPass->Name);
				{
					RenderPass->Execution(PassRecorder);
				}
				CmdRecorder->EndRenderPass(PassRecorder);
			}

		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});
	}

}