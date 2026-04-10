#include "CommonHeader.h"
#include "RenderRenderComp.h"
#include "GpuApi/GpuResourceHelper.h"
#include "GpuApi/GpuRhi.h"

using namespace FW;

namespace SH
{
	RenderRenderComp::RenderRenderComp(Render* InRenderGraph, PreviewViewPort* InViewPort)
		: RenderGraphAsset(InRenderGraph)
	{
		Context.ViewPort = InViewPort;
	}

	void RenderRenderComp::RenderBegin()
	{
		Context.Outputs.Reset();
	}

	void RenderRenderComp::RenderInternal()
	{
		if (!RenderGraphAsset || !Context.ViewPort)
		{
			return;
		}

		FIntPoint ViewSize = Context.ViewPort->GetSize();
		if (ViewSize.X == 0 || ViewSize.Y == 0)
		{
			return;
		}

		RenderGraph Graph;
		Context.RG = &Graph;
		if (!Context.FinalRT.IsValid() || Context.FinalRT->GetWidth() != ViewSize.X || Context.FinalRT->GetHeight() != ViewSize.Y)
		{
			Context.FinalRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ViewSize.X,
				.Height = (uint32)ViewSize.Y,
				.Format = GpuFormat::B8G8R8A8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			});
			GGpuRhi->SetResourceName("RenderFinalRT", Context.FinalRT);
			Context.ViewPort->SetViewPortRenderTexture(Context.FinalRT);
		}

		ExecRet GraphRet = RenderGraphAsset->Exec(Context);
		Context.Outputs.Sort([](const RenderOutputDesc& A, const RenderOutputDesc& B) {
			return A.Layer < B.Layer;
		});
		for (int32 Index = 0; Index < Context.Outputs.Num(); ++Index)
		{
			Context.Outputs[Index].Pass.LoadAction = Index == 0 ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load;
			AddBlitPass(*Context.RG, Context.Outputs[Index].Pass);
		}

		Graph.Execute();
		if (GraphRet.Terminate)
		{
			return;
		}
	}

	void RenderRenderComp::RenderEnd()
	{
	}
}
