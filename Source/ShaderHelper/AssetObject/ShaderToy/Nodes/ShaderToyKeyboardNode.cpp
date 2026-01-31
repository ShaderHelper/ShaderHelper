#include "CommonHeader.h"
#include "ShaderToyKeyboardNode.h"
#include "AssetObject/Pins/Pins.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderToyKeyboardNode>("Keyboard Node")
		.BaseClass<GraphNode>()
		.Data<&ShaderToyKeyboardNode::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
		.Data<&ShaderToyKeyboardNode::Width, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Width"))
		.Data<&ShaderToyKeyboardNode::Height, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Height"))
	)
	REFLECTION_REGISTER(AddClass<ShaderToyKeyboardNodeOp>()
		.BaseClass<ShObjectOp>()
	)
	REGISTER_NODE_TO_GRAPH(ShaderToyKeyboardNode, "ShaderToy Graph")

	MetaType* ShaderToyKeyboardNodeOp::SupportType()
	{
		return GetMetaType<ShaderToyKeyboardNode>();
	}

	void ShaderToyKeyboardNodeOp::OnCancelSelect(FW::ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty(true);
	}

	void ShaderToyKeyboardNodeOp::OnSelect(ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}

	ShaderToyKeyboardNode::ShaderToyKeyboardNode()
	: Format(GpuTextureFormat::R8_UNORM), Width(256), Height(3)
	{
		ObjectName = LOCALIZATION("Keyboard");
	}

	ShaderToyKeyboardNode::~ShaderToyKeyboardNode()
	{

	}

	void ShaderToyKeyboardNode::Init()
	{
		InitPins();
	}

	void ShaderToyKeyboardNode::InitPins()
	{
		auto OutputPin = NewShObject<GpuTexturePin>(this);
		OutputPin->ObjectName = FText::FromString("RT");
		OutputPin->Direction = PinDirection::Output;

		Pins = { MoveTemp(OutputPin) };
	}

	void ShaderToyKeyboardNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
	}

	ExecRet ShaderToyKeyboardNode::Exec(GraphExecContext& Context)
	{
		ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
		auto OutputPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		check(ShaderToyContext.Keyboard->GetFormat() == Format && ShaderToyContext.Keyboard->GetWidth() == Width && ShaderToyContext.Keyboard->GetHeight() == Height);
		OutputPin->SetValue(ShaderToyContext.Keyboard);
		return {};
	}
}
