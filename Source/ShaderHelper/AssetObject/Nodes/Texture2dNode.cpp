#include "CommonHeader.h"
#include "Texture2dNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/Pins/Pins.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "Renderer/RenderGraph.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture2dNode>("Texture2d Node")
		.BaseClass<GraphNode>()
		.Data<&Texture2dNode::Texture, MetaInfo::Property>("Texture")
		.Data<&Texture2dNode::Format, MetaInfo::Property | MetaInfo::ReadOnly>("Format")
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
		Ar << ChannelFilter;
	}

	void Texture2dNode::PostLoad()
	{
		GraphNode::PostLoad();
		
		InitTexture();
	}

	TSharedPtr<SWidget> Texture2dNode::ExtraNodeWidget()
	{
		return	SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
						[
							SNew(SShToggleButton).Text(FText::FromString("R"))
							.IsChecked_Lambda([this] { return ChannelFilter == TextureChannelFilter::R ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
								if (InState == ECheckBoxState::Checked)
								{
									ChannelFilter = TextureChannelFilter::R;
								}
								else
								{
									ChannelFilter = TextureChannelFilter::None;
								}
								RefershPreview();
							})
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
						[
							SNew(SShToggleButton).Text(FText::FromString("G"))
							.IsChecked_Lambda([this] { return ChannelFilter == TextureChannelFilter::G ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
								if (InState == ECheckBoxState::Checked)
								{
									ChannelFilter = TextureChannelFilter::G;
								}
								else
								{
									ChannelFilter = TextureChannelFilter::None;
								}
								RefershPreview();
							})
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
						[
							SNew(SShToggleButton).Text(FText::FromString("B"))
							.IsChecked_Lambda([this] { return ChannelFilter == TextureChannelFilter::B ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
								if (InState == ECheckBoxState::Checked)
								{
									ChannelFilter = TextureChannelFilter::B;
								}
								else
								{
									ChannelFilter = TextureChannelFilter::None;
								}
								RefershPreview();
							})
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
						[
							SNew(SShToggleButton).Text(FText::FromString("A"))
							.IsChecked_Lambda([this] { return ChannelFilter == TextureChannelFilter::A ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
								if (InState == ECheckBoxState::Checked)
								{
									ChannelFilter = TextureChannelFilter::A;
								}
								else
								{
									ChannelFilter = TextureChannelFilter::None;
								}
								RefershPreview();
							})
						]
					]
				]
				+ SVerticalBox::Slot()
				.Padding(4, 0 ,4 ,4)
				[
					SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{ 80, 80 })
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
		GpuTextureDesc Desc{ Width, Height, Format, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		TRefCountPtr<GpuTexture> PreviewTex = GGpuRhi->CreateTexture(MoveTemp(Desc));

		RenderGraph Graph;
		{
			BlitPassInput Input;
			Input.InputTex = Texture->GetGpuData();
			Input.InputTexSampler = GpuResourceHelper::GetSampler({});
			Input.OutputRenderTarget = PreviewTex;
			Input.VariantDefinitions.insert(FString::Printf(TEXT("CHANNEL_FILTER_%s"), ANSI_TO_TCHAR(magic_enum::enum_name(ChannelFilter).data())));
			AddBlitPass(Graph, MoveTemp(Input));
		}
		Graph.Execute();

		Preview->SetViewPortRenderTexture(PreviewTex);

	}

	void Texture2dNode::ClearProperty()
	{
		Width = Height = 0;
		auto ResultPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		ResultPin->SetValue(nullptr);
		Preview->Clear();
		
		PropertyDatas.Empty();
		ShObject::GetPropertyDatas();
		GetOuterMost()->MarkDirty();
		
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty();
	}

	void Texture2dNode::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		
		//Texture asset changed.
		if(InProperty->GetDisplayName() == "Texture")
		{
			InitTexture();
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
