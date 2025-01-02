#include "CommonHeader.h"
#include "BlitPass.h"
#include "RenderResource/Shader/BlitShader.h"

namespace FW
{

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput)
	{
		GpuRenderPassDesc BlitPassDesc;
		BlitPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassInput.OutputRenderTarget, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });

		BlitShader PassShader;

		BindingContext Binding;
		BlitShader::Parameters ShaderParameter{ PassInput.InputTex, PassInput.InputTexSampler };
		Binding.SetShaderBindGroup(PassShader.GetBindGroup(ShaderParameter));
		Binding.SetShaderBindGroupLayout(PassShader.GetBindGroupLayout());

		GpuRenderPipelineStateDesc PipelineDesc{
			PassShader.GetVertexShader(), 
			PassShader.GetPixelShader(),
			{ 
				{ PassInput.OutputRenderTarget->GetFormat() }
			}
		};
		Binding.ApplyBindGroupLayout(PipelineDesc);

		TRefCountPtr<GpuPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

		Graph.AddRenderPass("BlitPass", MoveTemp(BlitPassDesc),
			[Pipeline, Binding](GpuRenderPassRecorder* PassRecorder) {
				Binding.ApplyBindGroup(PassRecorder);
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
		);
	}

}