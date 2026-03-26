#include "CommonHeader.h"
#include "Texture3D.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuRhi.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace FW
{
	REFLECTION_REGISTER(AddClass<Texture3D>("Texture3D")
		.BaseClass<AssetObject>()
		.Data<&Texture3D::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
		.Data<&Texture3D::Width, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Width"))
		.Data<&Texture3D::Height, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Height"))
		.Data<&Texture3D::Depth, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Depth"))
		.Data<&Texture3D::GenerateMipmap, MetaInfo::Property>(LOCALIZATION("GenerateMipmap"))
	)

	Texture3D::Texture3D()
		: Width(0), Height(0), Depth(0), Format(GpuFormat::R8G8B8A8_UNORM)
	{
	}

	Texture3D::Texture3D(uint32 InWidth, uint32 InHeight, uint32 InDepth, GpuFormat InFormat, const TArray<uint8>& InRawData)
		: Width(InWidth)
		, Height(InHeight)
		, Depth(InDepth)
		, Format(InFormat)
		, RawData(InRawData)
	{
		InitGpuData();
	}

	void Texture3D::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << Format;
		Ar << Width;
		Ar << Height;
		Ar << Depth;
		Ar << RawData;
		Ar << GenerateMipmap;
	}

	void Texture3D::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpuData();
	}

	void Texture3D::PostPropertyChanged(PropertyData* InProperty)
	{
		AssetObject::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("GenerateMipmap")))
		{
			InitGpuData();
		}
	}

	void Texture3D::InitGpuData()
	{
		if (Width == 0 || Height == 0 || Depth == 0) return;

		GpuTextureDesc Desc;
		Desc.Width = Width;
		Desc.Height = Height;
		Desc.Depth = Depth;
		Desc.Format = Format;
		Desc.Usage = GpuTextureUsage::ShaderResource | (GenerateMipmap ? GpuTextureUsage::UnorderedAccess : GpuTextureUsage::None);
		Desc.Dimension = GpuTextureDimension::Tex3D;
		Desc.InitialData = RawData;
		Desc.NumMips = GenerateMipmap ? 0u : 1u;

		GpuData = GGpuRhi->CreateTexture(Desc, GpuResourceState::ShaderResourceRead);

		// Create a 2D shared texture from the first Z-slice for preview/thumbnail
		const uint32 BytesPerTexel = GetFormatByteSize(Format);
		const uint32 SliceByteSize = Width * Height * BytesPerTexel;
		const uint32 FirstSlice = 0;

		if (RawData.Num() >= (int32)((FirstSlice + 1) * SliceByteSize))
		{
			TArray<uint8> SliceData;
			SliceData.SetNum(SliceByteSize);
			FMemory::Memcpy(SliceData.GetData(), RawData.GetData() + FirstSlice * SliceByteSize, SliceByteSize);

			GpuTextureDesc PreviewDesc{ Width, Height, Format, GpuTextureUsage::ShaderResource | GpuTextureUsage::Shared, SliceData };
			PreviewData = GGpuRhi->CreateTexture(MoveTemp(PreviewDesc));
		}
	}

	FString Texture3D::FileExtension() const
	{
		return TEXT("texture3d");
	}

	TRefCountPtr<GpuTexture> Texture3D::GenerateThumbnail() const
	{
		return PreviewData;
	}
}
