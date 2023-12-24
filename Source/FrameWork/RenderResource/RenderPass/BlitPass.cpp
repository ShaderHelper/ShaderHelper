#include "CommonHeader.h"
#include "BlitPass.h"
#include "RenderResource/Shader/BlitShader.h"

namespace FRAMEWORK
{

	void AddBlitPass(RenderGraph& Graph, const BlitPassInput& PassInput)
	{
		GpuRenderPassDesc BlitPassDesc;
		BlitPassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassInput.OutputRenderTarget, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });

		TSharedPtr<BlitShader> PassShader;

		BindingContext Binding;
		BlitShader::Parameters ShaderParameter{ PassInput.InputTex, PassInput.InputTexSampler };
		Binding.SetShaderBindGroup(PassShader->GetBindGroup(ShaderParameter));
		Binding.SetShaderBindGroupLayout(PassShader->GetBindGroupLayout());

		GpuPipelineStateDesc PipelineDesc{
			PassShader->GetVertexShader(), 
			PassShader->GetPixelShader(),
			{ 
				{ PassInput.OutputRenderTarget->GetFormat() }
			}
		};
		Binding.ApplyBindGroupLayout(PipelineDesc);

		TRefCountPtr<GpuPipelineState> Pipeline = GpuApi::CreateRenderPipelineState(PipelineDesc);

		Graph.AddPass("BlitPass", MoveTemp(BlitPassDesc),
			[Pipeline, Binding] {
				Binding.ApplyBindGroup();
				GpuApi::SetRenderPipelineState(Pipeline);
				GpuApi::DrawPrimitive(0, 3, 0, 1);
			}
		);
	}

}