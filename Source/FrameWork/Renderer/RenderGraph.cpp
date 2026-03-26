#include "CommonHeader.h"
#include "RenderGraph.h"
#include "GpuApi/GpuBindGroup.h"

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
			InOutPass.PassResourceStates.Add(RtInfo.View, GpuResourceState::RenderTargetWrite);
		}

		InOutPass.Bindings.EnumerateBinding([&](const ResourceBinding& ResourceBindingEntry, const LayoutBinding& LayoutBindingEntry) {
			if (LayoutBindingEntry.Type == BindingType::Texture || LayoutBindingEntry.Type == BindingType::TextureCube || LayoutBindingEntry.Type == BindingType::Texture3D)
			{
				GpuResource* Res = ResourceBindingEntry.Resource.GetReference();
				InOutPass.PassResourceStates.Add(Res, GpuResourceState::ShaderResourceRead);
			}
			else if (LayoutBindingEntry.Type == BindingType::CombinedTextureSampler || LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler || LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler)
			{
				GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(ResourceBindingEntry.Resource.GetReference());
				InOutPass.PassResourceStates.Add(Combined->GetTexture(), GpuResourceState::ShaderResourceRead);
			}
			else if (LayoutBindingEntry.Type == BindingType::RWStructuredBuffer || LayoutBindingEntry.Type == BindingType::RWRawBuffer)
			{
				GpuBuffer* Buffer = static_cast<GpuBuffer*>(ResourceBindingEntry.Resource.GetReference());
				InOutPass.PassResourceStates.Add(Buffer, GpuResourceState::UnorderedAccess);
			}
			else if (LayoutBindingEntry.Type == BindingType::RWTexture || LayoutBindingEntry.Type == BindingType::RWTexture3D)
			{
				GpuResource* Res = ResourceBindingEntry.Resource.GetReference();
				InOutPass.PassResourceStates.Add(Res, GpuResourceState::UnorderedAccess);
			}
		});
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
		
		//Allow multiple executions?
		RGRenderPasses.Empty();
		CmdRecorder = GGpuRhi->BeginRecording();
	}

}
