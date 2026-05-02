#include "CommonHeader.h"
#include "BlitPass.h"
#include "RenderResource/Shader/BlitShader.h"

namespace FW
{

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput)
	{
		GpuRenderPassDesc BlitPassDesc;
		BlitPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassInput.OutputView, PassInput.LoadAction, RenderTargetStoreAction::Store });

		std::set<FString> VariantDefs = PassInput.VariantDefinitions;
		if (PassInput.MipLevel >= 0)
		{
			VariantDefs.insert(TEXT("USE_MIP_LEVEL"));
		}

		BlitShader* PassShader = GetShader<BlitShader>(VariantDefs);

		BlitShader::Parameters ShaderParameter{ PassInput.InputView, PassInput.InputTexSampler, (float)PassInput.MipLevel };
		TRefCountPtr<GpuBindGroup> ShaderBindGroup = PassShader->GetBindGroup(ShaderParameter);

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = PassShader->GetVertexShader(),
			.Ps = PassShader->GetPixelShader(),
			.Targets = {
				{ .TargetFormat = PassInput.OutputView->GetTexture()->GetFormat() }
			},
			.BindGroupLayouts = { PassShader->GetBindGroupLayout() },
		};

		TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

		Graph.AddRenderPass("BlitPass", MoveTemp(BlitPassDesc),
			[Pipeline, ShaderBindGroup, Scissor = PassInput.Scissor, Viewport = PassInput.Viewport](GpuRenderPassRecorder* PassRecorder) {
				PassRecorder->SetBindGroups({ ShaderBindGroup.GetReference() });
				PassRecorder->SetRenderPipelineState(Pipeline);
				if (Viewport)
				{
					PassRecorder->SetViewPort(Viewport.GetValue());
				}
				if (Scissor)
				{
					PassRecorder->SetScissorRect(Scissor.GetValue());
				}
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
		).Read(PassInput.InputView);
	}

}
