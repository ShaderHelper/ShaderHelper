#pragma once
#include "AssetImporter.h"

namespace FRAMEWORK
{
	class AssetTextureImporter : public AssetImporter
	{
	public:
		TArray<FString> SupportFileExts() const override;
	};
}