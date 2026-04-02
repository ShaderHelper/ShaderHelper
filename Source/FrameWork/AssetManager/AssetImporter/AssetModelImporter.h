#pragma once
#include "AssetImporter.h"

namespace FW
{
	class AssetModelImporter : public AssetImporter
	{
		REFLECTION_TYPE(AssetModelImporter)
	public:
		TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) override;
		TArray<FString> SupportFileExts() const override;
		MetaType* SupportAsset() override;
	};
}
