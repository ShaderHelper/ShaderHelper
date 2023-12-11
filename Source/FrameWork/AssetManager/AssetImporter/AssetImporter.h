#pragma once

namespace FRAMEWORK
{
	class AssetImporter
	{
	public:
		AssetImporter() = default;
		virtual ~AssetImporter() = default;

		virtual TArray<FString> SupportFileExts() const = 0;
	};
}