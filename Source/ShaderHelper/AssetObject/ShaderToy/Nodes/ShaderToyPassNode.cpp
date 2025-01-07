#include "CommonHeader.h"
#include "ShaderToyPassNode.h"
#include "Renderer/ShaderToyRenderComp.h"
#include <Widgets/SViewport.h>

using namespace FW;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToyPassNode>("RenderPass Node")
		.BaseClass<GraphNode>()
	)

	ShaderToyPassNode::ShaderToyPassNode()
	{
		ObjectName = FText::FromString("RenderPass");
	}

	void ShaderToyPassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		
		Slot0.Serialize(Ar);
		PassOutput.Serialize(Ar);
	}

	TSharedPtr<SWidget> ShaderToyPassNode::ExtraNodeWidget()
	{
		Preview = MakeShared<PreviewViewPort>();
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{ 80, 80 })
			];
	}

	TArray<GraphPin*> ShaderToyPassNode::GetPins()
	{
		return { &PassOutput,&Slot0};
	}

	void ShaderToyPassNode::SetPassShader(AssetPtr<StShader> InPassShader)
	{
		PassShader = MoveTemp(InPassShader);

		VertexShader = GGpuRhi->CreateShaderFromSource(ShaderType::VertexShader, PassShader->GetFullShader(), FString::Printf(TEXT("%sVS"), *ObjectName.ToString()), TEXT("MainVS"));
		FString ErrorInfo;
		GGpuRhi->CrossCompileShader(VertexShader, ErrorInfo);
		check(ErrorInfo.IsEmpty());

		PixelShader = GGpuRhi->CreateShaderFromSource(ShaderType::PixelShader, PassShader->GetFullShader(), FString::Printf(TEXT("%sPS"), *ObjectName.ToString()), TEXT("MainPS"));
		GGpuRhi->CrossCompileShader(PixelShader, ErrorInfo);
		check(ErrorInfo.IsEmpty());
	}


	void ShaderToyPassNode::Exec(GraphExecContext& Context)
	{
		if (!PassShader.IsValid())
		{
			return;
		}

		ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
		PassShader->BuiltInUniformBuffer->GetMember<Vector2f>("iResolution") = ShaderToyContext.iResolution;
		PassShader->BuiltInUniformBuffer->GetMember<float>("iTime") = ShaderToyContext.iTime;

		GpuTextureDesc Desc{ (uint32)ShaderToyContext.iResolution.x, (uint32)ShaderToyContext.iResolution.y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		TRefCountPtr<GpuTexture> PassOuputTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		Preview->SetViewPortRenderTexture(PassOuputTex);
		PassOutput.SetValue(PassOuputTex);

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassOuputTex, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });

		GpuRenderPipelineStateDesc PipelineDesc{
			//Shader
			VertexShader,
			PixelShader,
			//Targets
			{
				{ PassOuputTex->GetFormat() }
			},
			//BindGroupLayout
			{ PassShader->BuiltInBindGroupLayout }
		};
		TRefCountPtr<GpuPipelineState> PipelineState = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

		ShaderToyContext.RG->AddRenderPass(ObjectName.ToString(), MoveTemp(PassDesc),
			[this, PipelineState, &ShaderToyContext](GpuRenderPassRecorder* PassRecorder) {
				PassRecorder->SetRenderPipelineState(PipelineState);
				PassRecorder->SetBindGroups(PassShader->BuiltInBindGroup, nullptr, nullptr, nullptr);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
		);
	}

}