#include "CommonHeader.h"
#include "BlitPass.h"
#include "RenderResource/Shader/BlitShader.h"

namespace FW
{

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput)
	{
		GpuRenderPassDesc BlitPassDesc;
		BlitPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassInput.OutputRenderTarget, PassInput.LoadAction, RenderTargetStoreAction::Store });

		BlitShader* PassShader = GetShader<BlitShader>();

		BindingContext Bindings;
		BlitShader::Parameters ShaderParameter{ PassInput.InputTex, PassInput.InputTexSampler };
		Bindings.SetShaderBindGroup(PassShader->GetBindGroup(ShaderParameter));
		Bindings.SetShaderBindGroupLayout(PassShader->GetBindGroupLayout());

		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = PassShader->GetVertexShader(),
			.Ps = PassShader->GetPixelShader(),
			.Targets = {
				{ .TargetFormat = PassInput.OutputRenderTarget->GetFormat() }
			}
		};
		Bindings.ApplyBindGroupLayout(PipelineDesc);

		TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

		Graph.AddRenderPass("BlitPass", MoveTemp(BlitPassDesc), MoveTemp(Bindings),
			[Pipeline, Scissor = PassInput.Scissor](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
				Bindings.ApplyBindGroup(PassRecorder);
				PassRecorder->SetRenderPipelineState(Pipeline);
				if (Scissor)
				{
					PassRecorder->SetScissorRect(Scissor.GetValue());
				}
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
		);
	}

}
