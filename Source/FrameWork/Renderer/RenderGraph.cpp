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
        RGRenderPasses.Add({PassName, PassInfo, InExecution});
	}

	void RenderGraph::Execute()
	{
		for (const auto& RenderPass: RGRenderPasses)
		{
            auto PassRecorder = CmdRecorder->BeginRenderPass(RenderPass.Desc, RenderPass.Name);
            {
                RenderPass.Execution(PassRecorder);
            }
            CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});
	}

}
