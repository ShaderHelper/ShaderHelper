#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "RenderResource/RenderPass/BlitPass.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyOuputNode>("Present Node")
		.BaseClass<GraphNode>()
	)
    REFLECTION_REGISTER(AddClass<ShaderToyOuputNodeOp>()
        .BaseClass<ShObjectOp>()
    )

    MetaType* ShaderToyOuputNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyOuputNode>();
    }

    void ShaderToyOuputNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	ShaderToyOuputNode::ShaderToyOuputNode()
	{
		ObjectName = FText::FromString("Present");
	}

    ShaderToyOuputNode::~ShaderToyOuputNode()
    {

    }

    void ShaderToyOuputNode::InitPins()
    {
        auto ResultPin = NewShObject<GpuTexturePin>(this);
        ResultPin->ObjectName = FText::FromString("RT");
        ResultPin->Direction = PinDirection::Input;
        
        Pins.Add(MoveTemp(ResultPin));
    }

	void ShaderToyOuputNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
	}

    ExecRet ShaderToyOuputNode::Exec(GraphExecContext& Context)
	{
        ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
        
        auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
        
        BlitPassInput Input;
        Input.InputTex = ResultPin->GetValue();
        Input.InputTexSampler = GGpuRhi->CreateSampler({ SamplerFilter::Bilinear});
        Input.OutputRenderTarget = ShaderToyContext.FinalRT;

        AddBlitPass(*ShaderToyContext.RG, MoveTemp(Input));
        
        return {};
	}
}
