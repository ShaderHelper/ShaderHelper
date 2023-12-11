#include "CommonHeader.h"
#include "AssetTextureImporter.h"
#include "Common/Util/Reflection.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<AssetTextureImporter>()
						.BaseClass<AssetImporter>()
	)

	TArray<FString> AssetTextureImporter::SupportFileExts() const
	{
		return { "png", "jpeg", "tga" };
	}

}