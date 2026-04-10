#include "CommonHeader.h"
#include "Texture3dNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Pins/Pins.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture3dNode>("Texture3D Node")
		.BaseClass<GraphNode>()
		.Data<&Texture3dNode::Texture, MetaInfo::Property>(LOCALIZATION("Texture"))
	)
	REFLECTION_REGISTER(AddClass<Texture3dNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(Texture3dNode, "ShaderToy")
	REGISTER_NODE_TO_GRAPH(Texture3dNode, "Render")

	MetaType* Texture3dNodeOp::SupportType()
	{
		return GetMetaType<Texture3dNode>();
	}

	Texture3dNode::Texture3dNode()
	{
		ObjectName = LOCALIZATION("Texture3D");
	}

	Texture3dNode::Texture3dNode(FW::AssetPtr<FW::Texture3D> InTexture)
		: Texture(MoveTemp(InTexture))
	{
		ObjectName = FText::FromString(Texture->GetFileName());
		InitTexture();
	}

	Texture3dNode::~Texture3dNode()
	{
		if (Texture)
		{
			Texture->OnDestroy.RemoveAll(this);
		}
	}

	void Texture3dNode::Init()
	{
		InitPins();
	}

	void Texture3dNode::InitPins()
	{
		auto ResultPin = NewShObject<GpuTexture3DPin>(this);
		ResultPin->ObjectName = FText::FromString("RT");
		ResultPin->Direction = PinDirection::Output;

		Pins = {ResultPin};
	}

	void Texture3dNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << Texture;
	}

	void Texture3dNode::PostLoad()
	{
		GraphNode::PostLoad();
		InitTexture();
	}

	TSharedPtr<SWidget> Texture3dNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{80, 80})
			];
	}

	void Texture3dNode::InitTexture()
	{
		if (Texture)
		{
			Texture->OnDestroy.AddRaw(this, &Texture3dNode::ClearProperty);
			Format = Texture->GetFormat();
			Width = Texture->GetWidth();
			Height = Texture->GetHeight();
			Depth = Texture->GetDepth();
			RefreshPreview();
		}
	}

	void Texture3dNode::RefreshPreview()
	{
		if (!Texture || !Texture->GetPreviewTexture())
		{
			return;
		}

		Preview->SetViewPortRenderTexture(Texture->GetPreviewTexture());
	}

	void Texture3dNode::ClearProperty()
	{
		Width = Height = Depth = 0;
		Format = GpuFormat::B8G8R8A8_UNORM;
		auto ResultPin = static_cast<GpuTexture3DPin*>(GetPin("RT"));
		ResultPin->SetValue(nullptr);
		Preview->Clear();
		GetOuterMost()->MarkDirty();
		static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
	}

	void Texture3dNode::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);

		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("Texture")))
		{
			InitTexture();
			static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
		}
	}

	ExecRet Texture3dNode::Exec(GraphExecContext& Context)
	{
		if (Texture)
		{
			auto ResultPin = static_cast<GpuTexture3DPin*>(GetPin("RT"));
			ResultPin->SetValue(Texture->GetGpuData());
		}
		return {};
	}
}
