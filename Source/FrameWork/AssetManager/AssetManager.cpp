#include "CommonHeader.h"
#include "AssetManager.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{

	void AssetManager::MountProject(const FString& InProjectContentDir)
	{
		TArray<FString> FileNames;
		IFileManager::Get().FindFilesRecursive(FileNames, *InProjectContentDir, TEXT("*"), true, false);
		TArray<FString> ManagedExts = GetManageredExts();
		for (const FString& FileName : FileNames)
		{
			if (ManagedExts.Contains(FPaths::GetExtension(FileName)))
			{
				TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*FileName));
				FGuid Guid;
				*Ar << Guid;

                GuidToPath.Add(Guid, FileName);
			}
		
		}
	}

	void AssetManager::UpdateGuidToPath(const FString& InPath)
	{
        TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InPath));
        FGuid Guid;
        *Ar << Guid;
        GuidToPath.FindOrAdd(Guid) = InPath;
	}

    void AssetManager::RemoveGuidToPath(const FString& InPath)
    {
        FGuid Guid = TSingleton<AssetManager>::Get().GetGuid(InPath);
        GuidToPath.Remove(Guid);
    }

	FString AssetManager::GetPath(const FGuid& InGuid) const
	{
		check(IsValidAsset(InGuid));
        return GuidToPath[InGuid];
	}

	FGuid AssetManager::GetGuid(const FString& InPath) const
	{
        check(IsValidAsset(InPath));
        return *GuidToPath.FindKey(InPath);
	}

	void AssetManager::AddAssetThumbnail(const FGuid& InGuid, TRefCountPtr<GpuTexture> InThumbnail)
	{
		AssetThumbnailPool.Add(InGuid, InThumbnail);
	}

	GpuTexture* AssetManager::FindAssetThumbnail(const FGuid& InGuid) const
	{
		if (AssetThumbnailPool.Contains(InGuid))
		{
			return AssetThumbnailPool[InGuid];
		}

		return nullptr;
	}

	void AssetManager::Clear()
	{
        GuidToPath.Empty();
		AssetRefCounts.Empty();
		AssetThumbnailPool.Empty();

		for (auto [_, AssetObjectPtr] : Assets)
		{
			delete AssetObjectPtr;
		}

		Assets.Empty();
	}

	TArray<FString> AssetManager::GetManageredExts() const
	{
		TArray<FString> ManageredExts;
        ForEachDefaultObject<AssetObject>([&](AssetObject* CurAssetObject){
            ManageredExts.Add(CurAssetObject->FileExtension());
        });
		return ManageredExts;
	}

	void AssetManager::AddRef(AssetObject* InAssetObject)
	{
		if (!AssetRefCounts.Contains(InAssetObject))
		{
			check(Assets.FindKey(InAssetObject));
			AssetRefCounts.Add(InAssetObject, 1);
		}
		else
		{
			AssetRefCounts[InAssetObject]++;
		}
	}

	void AssetManager::ReleaseRef(AssetObject* InAssetObject)
	{
		if (AssetRefCounts[InAssetObject] == 1)
		{
			FGuid Guid = *Assets.FindKey(InAssetObject);
			Assets.Remove(Guid);
			AssetRefCounts.Remove(InAssetObject);
			delete InAssetObject;
		}
		else
		{
			AssetRefCounts[InAssetObject]--;
		}
	}

}
