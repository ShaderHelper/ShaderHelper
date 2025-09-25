#pragma once
#include "AssetObject/AssetObject.h"

namespace FW
{
	class AssetImporter
	{
		REFLECTION_TYPE(AssetImporter)
	public:
		AssetImporter() = default;
		virtual ~AssetImporter() = default;

		virtual TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) = 0;
		virtual TArray<FString> SupportFileExts() const = 0;
        
        virtual MetaType* SupportAsset() = 0;
	};

	FRAMEWORK_API AssetImporter* GetAssetImporter(const FString& InPath);
}
