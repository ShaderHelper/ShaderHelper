#pragma once
#include "AssetManager/AssetObject.h"

namespace FRAMEWORK
{
	class AssetImporter
	{
	public:
		AssetImporter() = default;
		virtual ~AssetImporter() = default;

		virtual TUniquePtr<AssetObject> CreateAssetObject(const FString& InFilePath) = 0;
		virtual TArray<FString> SupportFileExts() const = 0;
	};
}