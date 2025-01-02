#pragma once
#include "AssetImporter.h"

namespace FW
{
	class AssetTextureDefaultImporter : public AssetImporter
	{
		REFLECTION_TYPE(AssetTextureDefaultImporter)
	public:
		TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) override;
		TArray<FString> SupportFileExts() const override;
        
        struct MetaType* SupportAsset() override;
        
	};
}
