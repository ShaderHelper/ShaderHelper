#include "CommonHeader.h"
#include "Texture2D.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuApiInterface.h"
#include "AssetManager/AssetManager.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/RenderPass/BlitPass.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<Texture2D>()
						.BaseClass<AssetObject>();
	)

	Texture2D::Texture2D()
		: Width(0), Height(0)
	{

	}

	Texture2D::Texture2D(uint32 InWidth, uint32 InHeight, const TArray<uint8>& InRawData)
		: Width(InWidth)
		, Height(InHeight)
		, RawData(InRawData)
	{
		
	}

	void Texture2D::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << Width;
		Ar << Height;
		Ar << RawData;
	}

	void Texture2D::PostLoad()
	{
		GpuTextureDesc Desc{ (uint32)Width, (uint32)Height, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource , RawData };
		GpuData = GpuApi::CreateTexture(MoveTemp(Desc));
	}

	FString Texture2D::FileExtension() const
	{
		return "Texture";
	}

	GpuTexture* Texture2D::GetThumbnail() const
	{
		check(GpuData.IsValid());

		if (GpuTexture* Thumbnail = TSingleton<AssetManager>::Get().FindAssetThumbnail(Guid))
		{
			return Thumbnail;
		}

		GpuTextureDesc Desc{ Width / 2, Height / 2, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
		TRefCountPtr<GpuTexture> Thumbnail = GpuApi::CreateTexture(MoveTemp(Desc));

		RenderGraph Graph;
		{
			BlitPassInput Input;
			Input.InputTex = GpuData;
			Input.InputTexSampler = GpuApi::CreateSampler({ SamplerFilter::Bilinear});
			Input.OutputRenderTarget = Thumbnail;

			AddBlitPass(Graph, MoveTemp(Input));
		}
		Graph.Execute();

		TSingleton<AssetManager>::Get().AddAssetThumbnail(Guid, Thumbnail);
		return Thumbnail;
	}
}
