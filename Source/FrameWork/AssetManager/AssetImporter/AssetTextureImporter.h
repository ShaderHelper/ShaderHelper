#pragma once
#include "AssetImporter.h"

namespace FRAMEWORK
{
	class AssetTextureImporter : public AssetImporter
	{
	public:
		TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) override;
		TArray<FString> SupportFileExts() const override;
	};
}