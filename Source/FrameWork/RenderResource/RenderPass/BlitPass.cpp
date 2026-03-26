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

		BindingContext Bindings;
		BlitShader::Parameters ShaderParameter{ PassInput.InputView, PassInput.InputTexSampler, (float)PassInput.MipLevel };
		Bindings.SetShaderBindGroup(PassShader->GetBindGroup(ShaderParameter));
		Bindings.SetShaderBindGroupLayout(PassShader->GetBindGroupLayout());

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = PassShader->GetVertexShader(),
			.Ps = PassShader->GetPixelShader(),
			.Targets = {
				{ .TargetFormat = PassInput.OutputView->GetTexture()->GetFormat() }
			}
		};
		Bindings.ApplyBindGroupLayout(PipelineDesc);

		TRefCountPtr<GpuRenderPipelineState> Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

		Graph.AddRenderPass("BlitPass", MoveTemp(BlitPassDesc), MoveTemp(Bindings),
			[Pipeline, Scissor = PassInput.Scissor, Viewport = PassInput.Viewport](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
				Bindings.ApplyBindGroup(PassRecorder);
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
		);
	}

}
