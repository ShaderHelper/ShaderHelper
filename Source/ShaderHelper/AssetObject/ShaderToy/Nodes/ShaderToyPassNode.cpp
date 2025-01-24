#include "CommonHeader.h"
#include "ShaderToyPassNode.h"
#include "Renderer/ShaderToyRenderComp.h"
#include <Widgets/SViewport.h>
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyPassNode>("ShaderPass Node")
		.BaseClass<GraphNode>()
        .Data<&ShaderToyPassNode::Shader, MetaInfo::Property>("Shader")
	)
    REFLECTION_REGISTER(AddClass<ShaderToyPassNodeOp>()
        .BaseClass<ShObjectOp>()
    )

    MetaType* ShaderToyPassNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyPassNode>();
    }

    void ShaderToyPassNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	ShaderToyPassNode::ShaderToyPassNode()
	{
		ObjectName = FText::FromString("ShaderPass");
	}

	void ShaderToyPassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		
		Slot0.Serialize(Ar);
		PassOutput.Serialize(Ar);
        Ar << Shader;
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
		return { &PassOutput,&Slot0,&Slot1,&Slot2,&Slot3};
	}

	bool ShaderToyPassNode::Exec(GraphExecContext& Context)
	{
		if (!Shader.IsValid())
		{
            SH_LOG(LogGraph, Error, TEXT("Node:%s does not specify the corresponding stShader."), *ObjectName.ToString());
            return false;
		}

//		ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
//		PassShader->BuiltInUniformBuffer->GetMember<Vector2f>("iResolution") = ShaderToyContext.iResolution;
//		PassShader->BuiltInUniformBuffer->GetMember<float>("iTime") = ShaderToyContext.iTime;
//
//		GpuTextureDesc Desc{ (uint32)ShaderToyContext.iResolution.x, (uint32)ShaderToyContext.iResolution.y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
//		TRefCountPtr<GpuTexture> PassOuputTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
//		Preview->SetViewPortRenderTexture(PassOuputTex);
//		PassOutput.SetValue(PassOuputTex);
//
//		GpuRenderPassDesc PassDesc;
//		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassOuputTex, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });
//
//		GpuRenderPipelineStateDesc PipelineDesc{
//			//Shader
//			VertexShader,
//			PixelShader,
//			//Targets
//			{
//				{ PassOuputTex->GetFormat() }
//			},
//			//BindGroupLayout
//			{ PassShader->BuiltInBindGroupLayout }
//		};
//		TRefCountPtr<GpuPipelineState> PipelineState = GGpuRhi->CreateRenderPipelineState(PipelineDesc);
//
//		ShaderToyContext.RG->AddRenderPass(ObjectName.ToString(), MoveTemp(PassDesc),
//			[this, PipelineState, &ShaderToyContext](GpuRenderPassRecorder* PassRecorder) {
//				PassRecorder->SetRenderPipelineState(PipelineState);
//				PassRecorder->SetBindGroups(PassShader->BuiltInBindGroup, nullptr, nullptr, nullptr);
//				PassRecorder->DrawPrimitive(0, 3, 0, 1);
//			}
//		);
        return true;
	}

}
