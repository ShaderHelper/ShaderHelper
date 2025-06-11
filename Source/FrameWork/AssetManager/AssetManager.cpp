#include "CommonHeader.h"
#include "AssetManager.h"
#include "GpuApi/GpuResource.h"

namespace FW
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
                FGuid Guid = ReadAssetGuidInDisk(FileName);
                GuidToPath.Add(Guid, FileName);
			}
		
		}
	}

    FGuid AssetManager::ReadAssetGuidInDisk(const FString& InPath)
    {
        //ps: Guid must be first entry in the asset binary.
        TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InPath));
        FGuid Guid;
        *Ar << Guid;
        return Guid;
    }

	void AssetManager::UpdateGuidToPath(const FString& InPath)
	{
        FGuid Guid = ReadAssetGuidInDisk(InPath);
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
		for(auto [Id, Asset] : Assets)
		{
			if(Asset->GetRefCount() > 0)
			{
				Asset->Destroy();
			}
		}
		Assets.Empty();
        GuidToPath.Empty();
		AssetThumbnailPool.Empty();
		
	}

	void AssetManager::DestroyAsset(const FString& InAssetPath)
	{
		if (AssetObject** Ptr = Assets.Find(GetGuid(InAssetPath)))
		{
            (*Ptr)->Destroy();
		}
	}

    void AssetManager::RemoveAsset(AssetObject* InAsset)
    {
        if(auto KeyPtr = Assets.FindKey(InAsset))
        {
            Assets.Remove(*KeyPtr);
        }

    }

	TArray<FString> AssetManager::GetManageredExts() const
	{
		TArray<FString> ManageredExts;
        ForEachDefaultObject<AssetObject>([&](AssetObject* CurAssetObject){
            ManageredExts.Add(CurAssetObject->FileExtension());
        });
		return ManageredExts;
	}

}
