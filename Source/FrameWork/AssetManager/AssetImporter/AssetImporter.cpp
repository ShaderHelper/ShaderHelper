#include "CommonHeader.h"
#include "AssetImporter.h"
#include "Common/Util/Reflection.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<AssetImporter>())

    AssetImporter* GetAssetImporter(const FString& InPath)
    {
		AssetImporter* FinalImporter = nullptr;
		TArray<AssetImporter*> AssetImporters;
		ForEachDefaultObject<AssetImporter>([&](AssetImporter* CurImporter) {
			AssetImporters.Add(CurImporter);
		});
		FString FileExt = FPaths::GetExtension(InPath);
		for (AssetImporter* AssetImporterPtr : AssetImporters)
		{
			if (AssetImporterPtr->SupportFileExts().Contains(FileExt))
			{
				FinalImporter = AssetImporterPtr;
				break;
			}
		}
        return FinalImporter;
    }
}
