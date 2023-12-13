#include "CommonHeader.h"
#include "AssetTextureImporter.h"
#include "Common/Util/Reflection.h"
#include <ImageWrapper/IImageWrapperModule.h>
#include <ImageWrapper/IImageWrapper.h>
#include <ImageWrapper/ImageWrapperHelper.h>
#include <Misc/FileHelper.h>
#include "Renderer/RenderResource/Texture2D.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<AssetTextureImporter>()
						.BaseClass<AssetImporter>()
	)

	TUniquePtr<AssetObject> AssetTextureImporter::CreateAssetObject(const FString& InFilePath)
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

	TArray<FString> AssetTextureImporter::SupportFileExts() const
	{
		return { "png", "jpeg", "jpg", "tga" };
	}

}