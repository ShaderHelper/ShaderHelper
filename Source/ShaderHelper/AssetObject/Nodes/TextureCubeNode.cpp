#include "CommonHeader.h"
#include "TextureCubeNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Pins/Pins.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<TextureCubeNode>("TextureCube Node")
		.BaseClass<GraphNode>()
		.Data<&TextureCubeNode::Texture, MetaInfo::Property>(LOCALIZATION("Texture"))
	)
	REFLECTION_REGISTER(AddClass<TextureCubeNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(TextureCubeNode, "ShaderToy Graph")

	MetaType* TextureCubeNodeOp::SupportType()
	{
		return GetMetaType<TextureCubeNode>();
	}

	TextureCubeNode::TextureCubeNode()
	{
		ObjectName = LOCALIZATION("TextureCube");
	}

	TextureCubeNode::TextureCubeNode(FW::AssetPtr<FW::TextureCube> InTexture)
		: Texture(MoveTemp(InTexture))
	{
		ObjectName = FText::FromString(Texture->GetFileName());
		InitTexture();
	}

	TextureCubeNode::~TextureCubeNode()
	{
		if (Texture)
		{
			Texture->OnDestroy.RemoveAll(this);
		}
	}

	void TextureCubeNode::Init()
	{
		InitPins();
	}

	void TextureCubeNode::InitPins()
	{
		auto ResultPin = NewShObject<GpuCubemapPin>(this);
		ResultPin->ObjectName = FText::FromString("RT");
		ResultPin->Direction = PinDirection::Output;

		Pins = {ResultPin};
	}

	void TextureCubeNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << Texture;
	}

	void TextureCubeNode::PostLoad()
	{
		GraphNode::PostLoad();
		InitTexture();
	}

	TSharedPtr<SWidget> TextureCubeNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{80, 80})
			];
	}

	void TextureCubeNode::InitTexture()
	{
		if (Texture)
		{
			Texture->OnDestroy.AddRaw(this, &TextureCubeNode::ClearProperty);
			Format = Texture->GetFormat();
			Size = Texture->GetSize();
			RefreshPreview();
		}
	}

	void TextureCubeNode::RefreshPreview()
	{
		if (!Texture || !Texture->GetPreviewTexture())
		{
			return;
		}

		Preview->SetViewPortRenderTexture(Texture->GetPreviewTexture());
	}

	void TextureCubeNode::RefreshProperty()
	{
		PropertyDatas.Empty();
		ShObject::GetPropertyDatas();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty();
	}

	void TextureCubeNode::ClearProperty()
	{
		Size = 0;
		Format = GpuFormat::B8G8R8A8_UNORM;
		auto ResultPin = static_cast<GpuCubemapPin*>(GetPin("RT"));
		ResultPin->SetValue(nullptr);
		Preview->Clear();
		GetOuterMost()->MarkDirty();
		RefreshProperty();
	}

	void TextureCubeNode::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);

		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("Texture")))
		{
			InitTexture();
			RefreshProperty();
		}
	}

	ExecRet TextureCubeNode::Exec(GraphExecContext& Context)
	{
		if (Texture)
		{
			auto ResultPin = static_cast<GpuCubemapPin*>(GetPin("RT"));
			ResultPin->SetValue(Texture->GetGpuData());
		}
		return {};
	}
}
