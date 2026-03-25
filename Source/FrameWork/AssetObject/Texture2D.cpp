#include "CommonHeader.h"
#include "Texture2D.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuRhi.h"
#include "AssetManager/AssetManager.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<Texture2D>("Texture2D")
                                .BaseClass<AssetObject>()
								.Data<&Texture2D::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
								.Data<&Texture2D::Width, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Width"))
								.Data<&Texture2D::Height, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Height"))
								.Data<&Texture2D::GenerateMipmap, MetaInfo::Property>(LOCALIZATION("GenerateMipmap"))
	)

	Texture2D::Texture2D()
		: Width(0), Height(0), Format(GpuFormat::B8G8R8A8_UNORM)
	{

	}

	Texture2D::Texture2D(uint32 InWidth, uint32 InHeight, GpuFormat InFormat, const TArray<uint8>& InRawData)
		: Width(InWidth)
		, Height(InHeight)
		, Format(InFormat)
		, RawData(InRawData)
	{
		InitGpudata();
	}

	void Texture2D::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << Format;
		Ar << Width;
		Ar << Height;
		Ar << RawData;
		Ar << GenerateMipmap;
	}

	void Texture2D::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpudata();
	}

	void Texture2D::PostPropertyChanged(PropertyData* InProperty)
	{
		AssetObject::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("GenerateMipmap")))
		{
			InitGpudata();
		}
	}

	void Texture2D::InitGpudata()
	{
		GpuTextureDesc Desc{ 
			.Width = (uint32)Width, 
			.Height = (uint32)Height, 
			.Format = Format,
			.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget,
			.InitialData = RawData,
			.NumMips = GenerateMipmap ? 0u : 1u,
		};
		GpuData = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::ShaderResourceRead);

		GpuTextureDesc PreviewDesc{ 
			.Width = (uint32)Width, 
			.Height = (uint32)Height, 
			.Format = Format,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
			.InitialData = RawData,
		};
		PreviewTex = GGpuRhi->CreateTexture(MoveTemp(PreviewDesc));
	}

	FString Texture2D::FileExtension() const
	{
		return "texture";
	}

	TRefCountPtr<GpuTexture> Texture2D::GenerateThumbnail() const
	{
		check(GpuData.IsValid());

		GpuTextureDesc Desc{ Width / 2, Height / 2, Format, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		TRefCountPtr<GpuTexture> Thumbnail = GGpuRhi->CreateTexture(MoveTemp(Desc));

		RenderGraph Graph;
		{
			BlitPassInput Input;
			Input.InputView = GpuData->GetDefaultView();
			Input.InputTexSampler = GpuResourceHelper::GetSampler({.Filter = SamplerFilter::Bilinear});
			Input.OutputView = Thumbnail->GetDefaultView();

			AddBlitPass(Graph, MoveTemp(Input));
		}
		Graph.Execute();
		return Thumbnail;
	}
}
