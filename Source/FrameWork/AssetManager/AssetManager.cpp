#include "CommonHeader.h"
#include "AssetManager.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	int GAssetVer;
	FCriticalSection GAssetCS;

	void AssetManager::MountProject(const FString& InProjectContentDir)
	{
		TArray<FString> FileNames;
		IFileManager::Get().FindFilesRecursive(FileNames, *InProjectContentDir, TEXT("*"), true, false);
		TArray<FString> ManagedExts = GetManageredExts();
		for (const FString& FileName : FileNames)
		{
			if (ManagedExts.Contains(FPaths::GetExtension(FileName)))
			{
				AssetHeader Header = ReadAssetHeaderInDisk(FileName);
				GuidToPath.Add(Header.Guid, FileName);
				RegisterAssetDependencies(Header.Guid, Header.DependencyGuids);
			}
		
		}
	}

	AssetHeader AssetManager::ReadAssetHeaderInDisk(const FString& InPath)
    {
        TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InPath));
		AssetHeader Header;
		*Ar << Header;
		return Header;
    }

	void AssetManager::UpdateGuidToPath(const FString& InPath)
	{
		AssetHeader Header = ReadAssetHeaderInDisk(InPath);
		GuidToPath.FindOrAdd(Header.Guid) = InPath;
		RegisterAssetDependencies(Header.Guid, Header.DependencyGuids);
	}

    void AssetManager::RemoveGuidToPath(const FString& InPath)
    {
        FGuid Guid = TSingleton<AssetManager>::Get().GetGuid(InPath);
        GuidToPath.Remove(Guid);
		RegisterAssetDependencies(Guid, {});

		RemoveAssetThumbnail(Guid);
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

	TOptional<FGuid> AssetManager::TryGetGuid(const FString& InPath) const
	{
		if (const FGuid* Id = GuidToPath.FindKey(InPath))
		{
			return *Id;
		}
		return TOptional<FGuid>();
	}

	void AssetManager::AddAssetThumbnail(const FGuid& InGuid, TRefCountPtr<GpuTexture> InThumbnail)
	{
		AssetThumbnailPool.Add(InGuid, InThumbnail);
	}

	void AssetManager::RemoveAssetThumbnail(const FGuid& InGuid)
	{
		AssetThumbnailPool.Remove(InGuid);
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
		AssetDependents.Empty();
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

	void AssetManager::RegisterAssetDependencies(const FGuid& DependentGuid, const TArray<FGuid>& DependencyGuids)
	{
		for (auto& [Guid, Dependents] : AssetDependents)
		{
			Dependents.Remove(DependentGuid);
		}

		for (const FGuid& DependencyGuid : DependencyGuids)
		{
			if (DependencyGuid.IsValid())
			{
				AssetDependents.FindOrAdd(DependencyGuid).Add(DependentGuid);
			}
		}
	}

	TSet<FGuid> AssetManager::GetAssetDependents(const FGuid& DependencyGuid) const
	{
		if (const TSet<FGuid>* Dependents = AssetDependents.Find(DependencyGuid))
		{
			return *Dependents;
		}

		return {};
	}

}
