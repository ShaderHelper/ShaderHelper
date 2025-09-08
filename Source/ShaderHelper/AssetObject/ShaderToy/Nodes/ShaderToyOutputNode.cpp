#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "AssetObject/Pins/Pins.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyOutputNode>("Present Node")
		.BaseClass<GraphNode>()
		.Data<&ShaderToyOutputNode::Format, MetaInfo::Property>("Format")
	)
    REFLECTION_REGISTER(AddClass<ShaderToyOutputNodeOp>()
        .BaseClass<ShObjectOp>()
    )

	REGISTER_NODE_TO_GRAPH(ShaderToyOutputNode, "ShaderToy Graph")

    MetaType* ShaderToyOutputNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyOutputNode>();
    }

    void ShaderToyOutputNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	ShaderToyOutputNode::ShaderToyOutputNode()
	: Format(ShaderToyFormat::B8G8R8A8_UNORM)
	{
		ObjectName = FText::FromString("Present");
	}

    ShaderToyOutputNode::~ShaderToyOutputNode()
    {

    }

    void ShaderToyOutputNode::InitPins()
    {
        auto ResultPin = NewShObject<GpuTexturePin>(this);
        ResultPin->ObjectName = FText::FromString("RT");
        ResultPin->Direction = PinDirection::Input;
        
        Pins.Add(MoveTemp(ResultPin));
    }

	void ShaderToyOutputNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
	}

    ExecRet ShaderToyOutputNode::Exec(GraphExecContext& Context)
	{
        ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
        
        auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		
		if(!ShaderToyContext.FinalRT.IsValid() ||
		   ShaderToyContext.FinalRT->GetWidth() != (uint32)ShaderToyContext.iResolution.X || ShaderToyContext.FinalRT->GetHeight() != (uint32)ShaderToyContext.iResolution.Y )
		{
			ShaderToyContext.FinalRT = GGpuRhi->CreateTexture({
				.Width = (uint32)ShaderToyContext.iResolution.x,
				.Height = (uint32)ShaderToyContext.iResolution.y,
				.Format = (GpuTextureFormat)Format,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			});
			GGpuRhi->SetResourceName("FinalRT", ShaderToyContext.FinalRT);
			ShaderToyContext.ViewPort->SetViewPortRenderTexture(ShaderToyContext.FinalRT);
		}
	
        
        BlitPassInput Input;
        Input.InputTex = ResultPin->GetValue();
        Input.InputTexSampler = GpuResourceHelper::GetSampler({.Filter = SamplerFilter::Bilinear});
        Input.OutputRenderTarget = ShaderToyContext.FinalRT;

        AddBlitPass(*ShaderToyContext.RG, MoveTemp(Input));
        
        return {};
	}
}
