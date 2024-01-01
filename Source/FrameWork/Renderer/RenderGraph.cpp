#include "CommonHeader.h"
#include "RenderGraph.h"
#include "GpuApi/GpuApiInterface.h"

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
			GpuApi::BeginRenderPass(Pass.Info, Pass.Name);
			{
				Pass.Execution();
			}
			GpuApi::EndRenderPass();
		}
		GpuApi::Submit();
	}

}