#include "CommonHeader.h"
#include "AssetManager.h"

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

				PathToGuid.Add(FileName, Guid);
			}
		
		}
	}

	FString AssetManager::GetPath(const FGuid& InGuid) const
	{
		return *PathToGuid.FindKey(InGuid);
	}

	FGuid AssetManager::GetGuid(const FString& InPath) const
	{
		return PathToGuid[InPath];
	}

	void AssetManager::Clear()
	{
		PathToGuid.Empty();
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
		TArray<ShReflectToy::MetaType*> AssetObjectMetaTypes = ShReflectToy::GetMetaTypes<AssetObject>();
		for (auto MetaTypePtr : AssetObjectMetaTypes)
		{
			AssetObject* DefaultAssetObject = static_cast<AssetObject*>(MetaTypePtr->GetDefaultObject());
			if (DefaultAssetObject)
			{
				ManageredExts.Add(DefaultAssetObject->FileExtension());
			}
		}
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