#include "CommonHeader.h"
#include "Texture2dNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Pins/Pins.h"

#include <Widgets/SViewport.h>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture2dNode>("Texture2D Node")
		.BaseClass<GraphNode>()
		.Data<&Texture2dNode::Texture, MetaInfo::Property>(LOCALIZATION("Texture"))
	)
	REFLECTION_REGISTER(AddClass<Texture2dNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(Texture2dNode, "ShaderToy")

	MetaType* Texture2dNodeOp::SupportType()
	{
		return GetMetaType<Texture2dNode>();
	}

	Texture2dNode::Texture2dNode()
	{
		ObjectName = LOCALIZATION("Texture2D");
	}

	Texture2dNode::~Texture2dNode()
	{
		if(Texture)
		{
			Texture->OnDestroy.RemoveAll(this);
		}
	}

	Texture2dNode::Texture2dNode(FW::AssetPtr<FW::Texture2D> InTexture)
	: Texture(MoveTemp(InTexture))
	{
		ObjectName = FText::FromString(Texture->GetFileName());
		InitTexture();
	}

	void Texture2dNode::Init()
	{
		InitPins();
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
		
		InitTexture();
	}

	TSharedPtr<SWidget> Texture2dNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{80, 80})
			];
	}

	void Texture2dNode::InitTexture()
	{
		if(Texture)
		{
			Texture->OnDestroy.AddRaw(this, &Texture2dNode::ClearProperty);
			Format = Texture->GetFormat();
			Width = Texture->GetWidth();
			Height = Texture->GetHeight();
			RefershPreview();
		}
	}

	void Texture2dNode::RefershPreview()
	{
		if (!Texture || !Texture->GetPreviewTexture())
		{
			return;
		}

		Preview->SetViewPortRenderTexture(Texture->GetPreviewTexture());
	}

	void Texture2dNode::RefreshProprety()
	{
		PropertyDatas.Empty();
		ShObject::GetPropertyDatas();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty();
	}

	void Texture2dNode::ClearProperty()
	{
		Width = Height = 1;
		Format = GpuFormat::R8G8B8A8_UNORM;
		auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		ResultPin->SetValue(nullptr);
		Preview->Clear();
		GetOuterMost()->MarkDirty();
		RefreshProprety();
	}

	void Texture2dNode::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		
		//Texture asset changed.
		if(InProperty->GetDisplayName().EqualTo(LOCALIZATION("Texture")))
		{
			InitTexture();
			RefreshProprety();
		}
	}

	ExecRet Texture2dNode::Exec(GraphExecContext& Context)
	{
		if(Texture)
		{
			auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
			ResultPin->SetValue(Texture->GetGpuData());
		}
		return {};
	}
}
