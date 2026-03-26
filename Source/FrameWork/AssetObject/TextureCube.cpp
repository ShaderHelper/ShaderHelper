#include "CommonHeader.h"
#include "TextureCube.h"
#include "AssetManager/AssetManager.h"
#include "Common/Util/Reflection.h"
#include "GpuApi/GpuRhi.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"

namespace FW
{
	REFLECTION_REGISTER(AddClass<TextureCube>("TextureCube")
		.BaseClass<AssetObject>()
		.Data<&TextureCube::Format, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Format"))
		.Data<&TextureCube::Size, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("Size"))
		.Data<&TextureCube::GenerateMipmap, MetaInfo::Property>(LOCALIZATION("GenerateMipmap"))
	)

	TextureCube::TextureCube()
		: Size(0), Format(GpuFormat::R8G8B8A8_UNORM)
	{
		FaceData.SetNum(6);
	}

	TextureCube::TextureCube(uint32 InSize, GpuFormat InFormat, const TArray<TArray<uint8>>& InFaceData)
		: Size(InSize)
		, Format(InFormat)
		, FaceData(InFaceData)
	{
		check(FaceData.Num() == 6);
		InitGpuData();
	}

	void TextureCube::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << Format;
		Ar << Size;
		Ar << FaceData;
		Ar << GenerateMipmap;
	}

	void TextureCube::PostLoad()
	{
		AssetObject::PostLoad();
		InitGpuData();
	}

	void TextureCube::PostPropertyChanged(PropertyData* InProperty)
	{
		AssetObject::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("GenerateMipmap")))
		{
			InitGpuData();
		}
	}

	void TextureCube::InitGpuData()
	{
		if (Size == 0) return;

		const uint32 FaceByteSize = Size * Size * GetFormatByteSize(Format);

		TArray<uint8> AllFaceData;
		AllFaceData.SetNum(FaceByteSize * 6);
		for (int32 Face = 0; Face < 6; Face++)
		{
			if (FaceData[Face].Num() > 0) {
				FMemory::Memcpy(AllFaceData.GetData() + Face * FaceByteSize, FaceData[Face].GetData(), FaceByteSize);
			} else {
				FMemory::Memzero(AllFaceData.GetData() + Face * FaceByteSize, FaceByteSize);
			}
		}

		GpuTextureDesc Desc;
		Desc.Width = Size;
		Desc.Height = Size;
		Desc.Format = Format;
		Desc.Usage = GpuTextureUsage::ShaderResource | (GenerateMipmap ? GpuTextureUsage::UnorderedAccess : GpuTextureUsage::None);
		Desc.Dimension = GpuTextureDimension::TexCube;
		Desc.InitialData = AllFaceData;
		Desc.NumMips = GenerateMipmap ? 0u : 1u;

		GpuData = GGpuRhi->CreateTexture(Desc, GpuResourceState::ShaderResourceRead);

		// Create a 2D shared texture from face 0 for preview/thumbnail
		if (FaceData.Num() == 6 && FaceData[0].Num() > 0)
		{
			GpuTextureDesc PreviewDesc{ Size, Size, Format, GpuTextureUsage::ShaderResource | GpuTextureUsage::Shared, FaceData[0] };
			PreviewData = GGpuRhi->CreateTexture(MoveTemp(PreviewDesc));
		}
	}

	FString TextureCube::FileExtension() const
	{
		return TEXT("cubemap");
	}

	TRefCountPtr<GpuTexture> TextureCube::GenerateThumbnail() const
	{
		return PreviewData;
	}

	void TextureCube::SetFaceData(int32 FaceIndex, const TArray<uint8>& InData)
	{
		check(FaceIndex >= 0 && FaceIndex < 6);
		FaceData[FaceIndex] = InData;
	}

	void TextureCube::SetData(uint32 InSize, GpuFormat InFormat, TArray<TArray<uint8>> InFaceData)
	{
		check(InFaceData.Num() == 6);
		Size = InSize;
		Format = InFormat;
		FaceData = MoveTemp(InFaceData);
		InitGpuData();
	}
}
