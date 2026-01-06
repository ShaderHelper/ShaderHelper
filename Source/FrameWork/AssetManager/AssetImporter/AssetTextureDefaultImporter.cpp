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
		TSharedPtr<IImageWrapper> ImageWrpper = ImageWrapperModule.CreateImageWrapper(ImportFormat);
		TArray<uint8> CompressedData;
		if (FFileHelper::LoadFileToArray(CompressedData, *InFilePath))
		{
			if (ImageWrpper.IsValid() && ImageWrpper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
			{
				ERGBFormat Format = ImageWrpper->GetFormat();
				TArray<uint8> UnCompressedData;
				if (Format == ERGBFormat::Gray)
				{
					ImageWrpper->GetRaw(ERGBFormat::Gray, 8, UnCompressedData);
					return MakeUnique<Texture2D>(ImageWrpper->GetWidth(), ImageWrpper->GetHeight(), GpuTextureFormat::R8_UNORM, UnCompressedData);
				}
				else
				{
					ImageWrpper->GetRaw(ERGBFormat::BGRA, 8, UnCompressedData);
					return MakeUnique<Texture2D>(ImageWrpper->GetWidth(), ImageWrpper->GetHeight(), GpuTextureFormat::B8G8R8A8_UNORM, UnCompressedData);
				}
			}
		}

		return nullptr;
	}

	TArray<FString> AssetTextureDefaultImporter::SupportFileExts() const
	{
		return { "png", "jpeg", "jpg", "tga" };
	}

	MetaType* AssetTextureDefaultImporter::SupportAsset()
	{
		return GetMetaType<Texture2D>();
	}

}
