#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "AssetObject/Pins/Pins.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyOutputNode>("Present Node")
		.BaseClass<GraphNode>()
		.Data<&ShaderToyOutputNode::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
		.Data<&ShaderToyOutputNode::Layer, MetaInfo::Property>(LOCALIZATION("Layer"))
		.Data<&ShaderToyOutputNode::AreaFraction, MetaInfo::Property>(LOCALIZATION("AreaFraction"), {
			.Min = [](const ShaderToyOutputNode*) { return 0.0f; },
			.Max = [](const ShaderToyOutputNode*) { return 1.0f; }
		})
	)
    REFLECTION_REGISTER(AddClass<ShaderToyOutputNodeOp>()
        .BaseClass<ShObjectOp>()
    )

	REGISTER_NODE_TO_GRAPH(ShaderToyOutputNode, "ShaderToy Graph")

    MetaType* ShaderToyOutputNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyOutputNode>();
    }

	void ShaderToyOutputNodeOp::OnCancelSelect(ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TSharedPtr<SShViewport> ViewportWidget = StaticCastSharedPtr<SShViewport>(ShEditor->GetViewPort()->GetAssociatedWidget().Pin());
		ViewportWidget->ResetSpliter();
	}

	void ShaderToyOutputNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
		ShEditor->ForceRender();

		TSharedPtr<SShViewport> ViewportWidget = StaticCastSharedPtr<SShViewport>(ShEditor->GetViewPort()->GetAssociatedWidget().Pin());
		ViewportWidget->SetSpliterFraction(TAttribute<float>::CreateLambda([InObject] {
			return static_cast<ShaderToyOutputNode*>(InObject)->AreaFraction;
		}));
		ViewportWidget->SetOnSpliterChanged([InObject, ShEditor](float InFraction) {
			static_cast<ShaderToyOutputNode*>(InObject)->AreaFraction = InFraction;
			ShEditor->ForceRender();
			InObject->GetOuterMost()->MarkDirty();
		});
    }

	ShaderToyOutputNode::ShaderToyOutputNode()
	: Format(ShaderToyFormat::B8G8R8A8_UNORM)
	, Layer(0), AreaFraction(1.0f)
	{
		ObjectName = LOCALIZATION("Present");
	}

    ShaderToyOutputNode::~ShaderToyOutputNode()
    {

    }

    void ShaderToyOutputNode::InitPins()
    {
        auto ResultPin = NewShObject<GpuTexturePin>(this);
        ResultPin->ObjectName = FText::FromString("RT");
        ResultPin->Direction = PinDirection::Input;
        
		Pins = { MoveTemp(ResultPin) };
    }

	void ShaderToyOutputNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << Layer;
		Ar << AreaFraction;
	}

    ExecRet ShaderToyOutputNode::Exec(GraphExecContext& Context)
	{
        ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
        
        auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));	
        
        BlitPassInput Input;
        Input.InputTex = ResultPin->GetValue();
        Input.InputTexSampler = GpuResourceHelper::GetSampler({});
        Input.OutputRenderTarget = ShaderToyContext.FinalRT;
		Input.Scissor = GpuScissorRectDesc{0, 0, (uint32)(AreaFraction * ShaderToyContext.FinalRT->GetWidth()), ShaderToyContext.FinalRT->GetHeight()};

		ShaderToyContext.Ontputs.Emplace(MoveTemp(Input), Layer);
        return {};
	}

	void ShaderToyOutputNode::PostPropertyChanged(PropertyData* InProperty)
	{
		GraphNode::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("AreaFraction")) || InProperty->GetDisplayName().EqualTo(LOCALIZATION("Layer")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ForceRender();
		}
	}
}
