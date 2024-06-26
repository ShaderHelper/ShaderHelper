#pragma once
#include "AssetImporter.h"

namespace FRAMEWORK
{
	class AssetTextureDefaultImporter : public AssetImporter
	{
	public:
		TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) override;
		TArray<FString> SupportFileExts() const override;
        
        struct MetaType* SupportAsset() override;
        
	};
}
