#include "CommonHeader.h"
#include "Texture2D.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuRhi.h"
#include "AssetManager/AssetManager.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/RenderPass/BlitPass.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<Texture2D>("Texture2D")
                                .BaseClass<AssetObject>()
	)

	Texture2D::Texture2D()
		: Width(0), Height(0), Format(GpuTextureFormat::B8G8R8A8_UNORM)
	{

	}

	Texture2D::Texture2D(uint32 InWidth, uint32 InHeight, GpuTextureFormat InFormat, const TArray<uint8>& InRawData)
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
	}

	void Texture2D::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpudata();
	}

	void Texture2D::InitGpudata()
	{
		GpuTextureDesc Desc{ 
			.Width = (uint32)Width, 
			.Height = (uint32)Height, 
			.Format = Format,
			.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared ,
			.InitialData = RawData,
			.NumMips = 0,
		};
		GpuData = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::ShaderResourceRead);
	}

	FString Texture2D::FileExtension() const
	{
		return "texture";
	}

	GpuTexture* Texture2D::GetThumbnail() const
	{
		check(GpuData.IsValid());

		if (GpuTexture* Thumbnail = TSingleton<AssetManager>::Get().FindAssetThumbnail(Guid))
		{
			return Thumbnail;
		}

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

		TSingleton<AssetManager>::Get().AddAssetThumbnail(Guid, Thumbnail);
		return Thumbnail;
	}
}
