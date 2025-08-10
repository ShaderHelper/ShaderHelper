#include "CommonHeader.h"
#include "Texture2dNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Pins/Pins.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture2dNode>("Texture2d Node")
		.BaseClass<GraphNode>()
		.Data<&Texture2dNode::Texture, MetaInfo::Property>("Texture")
		.Data<&Texture2dNode::Width, MetaInfo::Property | MetaInfo::ReadOnly>("Width")
		.Data<&Texture2dNode::Height, MetaInfo::Property | MetaInfo::ReadOnly>("Height")
	)
	REFLECTION_REGISTER(AddClass<Texture2dNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(Texture2dNode, "ShaderToy Graph")

	MetaType* Texture2dNodeOp::SupportType()
	{
		return GetMetaType<Texture2dNode>();
	}

	void Texture2dNodeOp::OnSelect(ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}

	Texture2dNode::Texture2dNode()
	{
		ObjectName = FText::FromString("Texture2d");
	}

	Texture2dNode::Texture2dNode(FW::AssetPtr<FW::Texture2D> InTexture)
	: Texture(MoveTemp(InTexture))
	, Width(Texture->GetWidth())
	, Height(Texture->GetHeight())
	{
		ObjectName = FText::FromString(Texture->GetFileName());
		Preview->SetViewPortRenderTexture(Texture->GetGpuData());
	}

	void Texture2dNode::InitPins()
	{
		auto ResultPin = NewShObject<GpuTexturePin>(this);
		ResultPin->ObjectName = FText::FromString("RT");
		ResultPin->Direction = PinDirection::Output;
		
		Pins = {ResultPin};
	}

	void Texture2dNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		
		Ar << Texture;
	}

	void Texture2dNode::PostLoad()
	{
		GraphNode::PostLoad();
		
		if(Texture)
		{
			Preview->SetViewPortRenderTexture(Texture->GetGpuData());
			Width = Texture->GetWidth();
			Height = Texture->GetHeight();
		}
	}

	TSharedPtr<SWidget> Texture2dNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{ 80, 80 })
			];
	}

	void Texture2dNode::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		
		//Texture asset changed.
		if(InProperty->GetDisplayName() == "Texture")
		{
			Preview->SetViewPortRenderTexture(Texture->GetGpuData());
			Width = Texture->GetWidth();
			Height = Texture->GetHeight();
		}
	}

	ExecRet Texture2dNode::Exec(GraphExecContext& Context)
	{
		auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		ResultPin->SetValue(Texture->GetGpuData());
		return {};
	}
}
