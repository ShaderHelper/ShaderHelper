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

	void RGPassBase::AddResourceState(GpuResource* InResource, GpuResourceState InState)
	{
		if (GpuResource* Resource = ResolveBarrierResource(InResource))
		{
			PassResourceStates.Add(Resource, InState);
		}
	}

	RGRenderPass& RGRenderPass::Read(GpuResource* InResource)
	{
		AddResourceState(InResource, GpuResourceState::ShaderResourceRead);
		return *this;
	}

	RGRenderPass& RGRenderPass::Write(GpuResource* InResource)
	{
		AddResourceState(InResource, GpuResourceState::UnorderedAccess);
		return *this;
	}

	void RGRenderPass::Execute(GpuCmdRecorder* CmdRecorder)
	{
		auto PassRecorder = CmdRecorder->BeginRenderPass(Desc, Name);
		{
			Execution(PassRecorder);
		}
		CmdRecorder->EndRenderPass(PassRecorder);
	}

	RGComputePass& RGComputePass::Read(GpuResource* InResource)
	{
		AddResourceState(InResource, GpuResourceState::ShaderResourceRead);
		return *this;
	}

	RGComputePass& RGComputePass::Write(GpuResource* InResource)
	{
		AddResourceState(InResource, GpuResourceState::UnorderedAccess);
		return *this;
	}

	void RGComputePass::Execute(GpuCmdRecorder* CmdRecorder)
	{
		auto PassRecorder = CmdRecorder->BeginComputePass(Name, TimestampWrites);
		{
			Execution(PassRecorder);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}

	RenderGraph::RenderGraph()
	{
		CmdRecorder = GGpuRhi->BeginRecording();
	}

	RenderGraph::RenderGraph(GpuCmdRecorder* InCmdRecorder) : CmdRecorder(InCmdRecorder)
	{

	}

	void RenderGraph::SetupRenderPass(RGRenderPass& InOutPass)
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

	void RenderGraph::CreatePassBarriers(const RGPassBase& InPass)
	{
		TArray<GpuBarrierInfo> BarrierInfos;
		for (auto [Res, DesiredState] : InPass.PassResourceStates)
		{
			BarrierInfos.Emplace(Res, DesiredState);
		}
		CmdRecorder->Barriers(BarrierInfos);
	}

	RGRenderPass& RenderGraph::AddRenderPass(const FString& PassName, GpuRenderPassDesc PassInfo, const RenderPassExecution& InExecution)
	{
		auto NewPass = MakeUnique<RGRenderPass>();
		NewPass->Name = PassName;
		NewPass->Desc = MoveTemp(PassInfo);
		NewPass->Execution = InExecution;
		SetupRenderPass(*NewPass);
		RGRenderPass& Ref = *NewPass;
		RGPasses.Add(MoveTemp(NewPass));
		return Ref;
	}

	RGComputePass& RenderGraph::AddComputePass(const FString& PassName, const ComputePassExecution& InExecution, TOptional<GpuPassTimestampWrites> TimestampWrites)
	{
		auto NewPass = MakeUnique<RGComputePass>();
		NewPass->Name = PassName;
		NewPass->Execution = InExecution;
		NewPass->TimestampWrites = MoveTemp(TimestampWrites);
		RGComputePass& Ref = *NewPass;
		RGPasses.Add(MoveTemp(NewPass));
		return Ref;
	}

	void RenderGraph::Execute()
	{
		for (auto& Pass : RGPasses)
		{
			CreatePassBarriers(*Pass);
			Pass->Execute(CmdRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({CmdRecorder});
		
		//Allow multiple executions?
		RGPasses.Empty();
		CmdRecorder = GGpuRhi->BeginRecording();
	}

}
