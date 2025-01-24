#include "CommonHeader.h"
#include "AssetTextureDefaultImporter.h"
#include "Common/Util/Reflection.h"
#include <IImageWrapperModule.h>
#include <IImageWrapper.h>
#include <ImageWrapperHelper.h>
#include <Misc/FileHelper.h>
#include "AssetObject/Texture2D.h"

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
				TArray<uint8> UnCompressedData;
				if (ImageWrpper->GetRaw(ERGBFormat::BGRA, 8, UnCompressedData))
				{
					return MakeUnique<Texture2D>(ImageWrpper->GetWidth(), ImageWrpper->GetHeight(), UnCompressedData);
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
