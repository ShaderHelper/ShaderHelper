#include "CommonHeader.h"
#include "RenderGraph.h"
#include "GpuApi/GpuRhi.h"

namespace FRAMEWORK
{

	void RenderGraph::AddPass(const FString& PassName, const GpuRenderPassDesc& PassInfo, const PassExecution& InExecution)
	{
		RGPasses.Add({ PassName, PassInfo, InExecution });
	}

	void RenderGraph::Execute()
	{
		for (const auto& Pass: RGPasses)
		{
			GGpuRhi->BeginRenderPass(Pass.Info, Pass.Name);
			{
				Pass.Execution();
			}
			GGpuRhi->EndRenderPass();
		}
		GGpuRhi->Submit();
	}

}