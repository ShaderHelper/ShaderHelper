#include "CommonHeader.h"
#include "RenderOutputNode.h"
#include "App/App.h"
#include "AssetObject/Pins/Pins.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/RenderRenderComp.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<RenderOutputNode>("RenderOutput Node")
		.BaseClass<GraphNode>()
		.Data<&RenderOutputNode::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
		.Data<&RenderOutputNode::Layer, MetaInfo::Property>(LOCALIZATION("Layer"))
		.Data<&RenderOutputNode::AreaFraction, MetaInfo::Property>(LOCALIZATION("AreaFraction"), {
			.Min = [](const RenderOutputNode*) { return 0.0f; },
			.Max = [](const RenderOutputNode*) { return 1.0f; }
		})
	)
	REFLECTION_REGISTER(AddClass<RenderOutputNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(RenderOutputNode, "Render")

	MetaType* RenderOutputNodeOp::SupportType()
	{
		return GetMetaType<RenderOutputNode>();
	}

	void RenderOutputNodeOp::OnCancelSelect(ShObject* InObject)
	{
		ShPropertyOp::OnCancelSelect(InObject);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TSharedPtr<SShViewport> ViewportWidget = StaticCastSharedPtr<SShViewport>(ShEditor->GetViewPort()->GetAssociatedWidget().Pin());
		ViewportWidget->ResetSpliter();
	}

	void RenderOutputNodeOp::OnSelect(ShObject* InObject)
	{
		ShPropertyOp::OnSelect(InObject);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();

		TSharedPtr<SShViewport> ViewportWidget = StaticCastSharedPtr<SShViewport>(ShEditor->GetViewPort()->GetAssociatedWidget().Pin());
		ViewportWidget->SetSpliterFraction(TAttribute<float>::CreateLambda([InObject] {
			return static_cast<RenderOutputNode*>(InObject)->AreaFraction;
		}));
		ViewportWidget->SetOnSpliterChanged([InObject, ShEditor](float InFraction) {
			static_cast<RenderOutputNode*>(InObject)->AreaFraction = InFraction;
			ShEditor->ForceRender();
			InObject->GetOuterMost()->MarkDirty();
		});
	}

	RenderOutputNode::RenderOutputNode()
		: Format(RenderFormat::B8G8R8A8_UNORM)
		, Layer(0)
		, AreaFraction(1.0f)
	{
		ObjectName = LOCALIZATION("Output");
	}

	RenderOutputNode::~RenderOutputNode()
	{
	}

	void RenderOutputNode::Init()
	{
		InitPins();
	}

	void RenderOutputNode::InitPins()
	{
		auto ResultPin = NewShObject<GpuTexturePin>(this);
		ResultPin->ObjectName = FText::FromString("RT");
		ResultPin->Direction = PinDirection::Input;

		Pins = { MoveTemp(ResultPin) };
	}

	void RenderOutputNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << Layer;
		Ar << AreaFraction;
	}

	ExecRet RenderOutputNode::Exec(GraphExecContext& Context)
	{
		RenderExecContext& RenderContext = static_cast<RenderExecContext&>(Context);
		auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		GpuTexture* ResultTexture = ResultPin->GetValue();
		if (!ResultTexture || !RenderContext.FinalRT.IsValid())
		{
			return {};
		}

		BlitPassInput Input;
		Input.InputView = ResultTexture->GetDefaultView();
		Input.InputTexSampler = GpuResourceHelper::GetSampler({});
		Input.OutputView = RenderContext.FinalRT->GetDefaultView();
		Input.Scissor = GpuScissorRectDesc{ 0, 0, (uint32)(AreaFraction * RenderContext.FinalRT->GetWidth()), RenderContext.FinalRT->GetHeight() };

		RenderContext.Outputs.Emplace(MoveTemp(Input), Layer);
		return {};
	}

	void RenderOutputNode::PostPropertyChanged(PropertyData* InProperty)
	{
		GraphNode::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("AreaFraction")) || InProperty->GetDisplayName().EqualTo(LOCALIZATION("Layer")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ForceRender();
		}
	}
}
