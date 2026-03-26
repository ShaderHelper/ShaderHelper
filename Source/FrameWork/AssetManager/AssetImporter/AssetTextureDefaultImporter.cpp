#include "CommonHeader.h"
#include "AssetTextureDefaultImporter.h"
#include "Common/Util/Reflection.h"
#include "AssetObject/Texture2D.h"

#include <IImageWrapperModule.h>
#include <IImageWrapper.h>
#include <ImageWrapperHelper.h>
#include <Misc/FileHelper.h>

namespace FW
{
    REFLECTION_REGISTER(AddClass<AssetTextureDefaultImporter>()
                                .BaseClass<AssetImporter>()
	)

	TUniquePtr<AssetObject> AssetTextureDefaultImporter::CreateAssetObject(const FString& InFilePath)
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
		EImageFormat ImportFormat = ImageWrapperHelper::GetImageFormat(FPaths::GetExtension(InFilePath));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImportFormat);
		TArray<uint8> CompressedData;
		if (FFileHelper::LoadFileToArray(CompressedData, *InFilePath))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
			{
				ERGBFormat Format = ImageWrapper->GetFormat();
				TArray<uint8> UnCompressedData;
				if (Format == ERGBFormat::Gray)
				{
					ImageWrapper->GetRaw(ERGBFormat::Gray, 8, UnCompressedData);
					return MakeUnique<Texture2D>(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), GpuFormat::R8_UNORM, UnCompressedData);
				}
				else
				{
					ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UnCompressedData);
					for (int32 PixelIndex = 0; PixelIndex + 3 < UnCompressedData.Num(); PixelIndex += 4)
					{
						Swap(UnCompressedData[PixelIndex], UnCompressedData[PixelIndex + 2]);
					}
					return MakeUnique<Texture2D>(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), GpuFormat::R8G8B8A8_UNORM, UnCompressedData);
				}
			}
		}

		return nullptr;
	}

	TArray<FString> AssetTextureDefaultImporter::SupportFileExts() const
	{
		return { "png", "jpeg", "jpg", "tga", "bmp" };
	}

	MetaType* AssetTextureDefaultImporter::SupportAsset()
	{
		return GetMetaType<Texture2D>();
	}

}
