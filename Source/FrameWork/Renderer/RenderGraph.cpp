#include "CommonHeader.h"
#include "RenderGraph.h"
#include "GpuApi/GpuBindGroup.h"

namespace FW
{
	static GpuResource* ResolveBarrierResource(GpuResource* InResource)
	{
		if (!InResource)
		{
			return nullptr;
		}

		if (InResource->GetType() == GpuResourceType::CombinedTextureSampler)
		{
			return static_cast<GpuCombinedTextureSampler*>(InResource)->GetTexture();
		}

		if (InResource->GetType() == GpuResourceType::Sampler)
		{
			return nullptr;
		}

		return InResource;
	}

	RGRenderPass& RGRenderPass::Read(GpuResource* InResource)
	{
		if (GpuResource* Resource = ResolveBarrierResource(InResource))
		{
			PassResourceStates.Add(Resource, GpuResourceState::ShaderResourceRead);
		}
		return *this;
	}

	RGRenderPass& RGRenderPass::Write(GpuResource* InResource)
	{
		if (GpuResource* Resource = ResolveBarrierResource(InResource))
		{
			PassResourceStates.Add(Resource, GpuResourceState::UnorderedAccess);
		}
		return *this;
	}

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
			InOutPass.PassResourceStates.Add(RtInfo.View, GpuResourceState::RenderTargetWrite);
		}
		if (InOutPass.Desc.DepthStencilTarget)
		{
			InOutPass.PassResourceStates.Add(InOutPass.Desc.DepthStencilTarget->View, GpuResourceState::DepthStencilWrite);
		}
	}

	void RenderGraph::CreatePassBarriers(const RGRenderPass& InPass)
	{
		TArray<GpuBarrierInfo> BarrierInfos;
		for (auto [Res, DesiredState] : InPass.PassResourceStates)
		{
			BarrierInfos.Emplace(Res, DesiredState);
		}
		CmdRecorder->Barriers(BarrierInfos);
	}

	void RenderGraph::ExecutePass(RGRenderPass& InPass)
	{
		auto PassRecorder = CmdRecorder->BeginRenderPass(InPass.Desc, InPass.Name);
		{
			InPass.Execution(PassRecorder);
		}
		CmdRecorder->EndRenderPass(PassRecorder);
	}

	RGRenderPass& RenderGraph::AddRenderPass(const FString& PassName, GpuRenderPassDesc PassInfo, const RenderPassExecution& InExecution)
	{
		RGRenderPass NewPass{ PassName, MoveTemp(PassInfo), InExecution };
		SetupPass(NewPass);
		int32 PassIndex = RGRenderPasses.Num();
        RGRenderPasses.Add(MoveTemp(NewPass));
		return RGRenderPasses[PassIndex];
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
		
		//Allow multiple executions?
		RGRenderPasses.Empty();
		CmdRecorder = GGpuRhi->BeginRecording();
	}

}
