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
			GpuResourceHelper::GDefaultRasterizerStateDesc,
			GpuResourceHelper::GDefaultBlendStateDesc,
			{ PassInput.OutputRenderTarget->GetFormat() }
		};
		Binding.ApplyBindGroupLayout(PipelineDesc);

		TRefCountPtr<GpuPipelineState> Pipeline = GpuApi::CreateRenderPipelineState(PipelineDesc);

		Graph.AddPass("BlitPass", MoveTemp(BlitPassDesc),
			[PassShader, Pipeline, Binding, ViewRect = PassInput.ViewRect] {
				GpuApi::SetRenderPipelineState(Pipeline);
				GpuApi::SetViewPort({ ViewRect.X,  ViewRect.Y });
				Binding.ApplyBindGroup();
				GpuApi::DrawPrimitive(0, 3, 0, 1);
			}
		);
	}

}