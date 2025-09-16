#pragma once
#include "AssetObject/AssetObject.h"

namespace FW
{
	class GpuTexture;

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<AssetObject, T>>>
    using AssetPtr = ObjectPtr<T, ObjectOwnerShip::Retain>;

	//Ensure the GuidToPath of AssetManager is always accessed with a normalized path.
	// A/B/Rsource.xxx = A/B\\Resource.xxx
	struct AssetPath
	{
		AssetPath() = default;
		AssetPath(FString InPathStr) : PathStr(MoveTemp(InPathStr)) 
		{
			FPaths::NormalizeFilename(PathStr);
			PathStr = FPaths::ConvertRelativePathToFull(PathStr);
			FPaths::RemoveDuplicateSlashes(PathStr);
		}

		bool operator==(const AssetPath& Other) const
		{
			return PathStr == Other.PathStr;
		}

		operator FString() const { return PathStr; }
		FString PathStr;
	};

	class FRAMEWORK_API AssetManager
	{
	public:
		void MountProject(const FString& InProjectContentDir);

		template<typename T>
		AssetPtr<T> LoadAssetByPath(const FString& InAssetPath)
		{
			static_assert(std::is_base_of_v<AssetObject, T>);
			if (!IsValidAsset(InAssetPath))
			{
				return nullptr;
			}

			FGuid Guid = GetGuid(InAssetPath);
			if (Assets.Contains(Guid))
			{
				return { static_cast<T*>(Assets[Guid]) };
			}

			AssetObject* NewAssetObject = nullptr;
			FString AssetExt = FPaths::GetExtension(InAssetPath);
			TArray<MetaType*> MetaTypes = GetMetaTypes<T>();
			for (auto MetaTypePtr : MetaTypes)
			{
				void* DefaultObject = MetaTypePtr->GetDefaultObject();
				if (DefaultObject)
				{
					T* RelDefaultObject = static_cast<T*>(DefaultObject);
					if (RelDefaultObject->FileExtension().Contains(AssetExt))
					{
						NewAssetObject = static_cast<AssetObject*>(MetaTypePtr->Construct());
						break;
					}
				}
			}
			check(NewAssetObject);
			TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InAssetPath));
            if(Ar)
            {
                NewAssetObject->Serialize(*Ar);
                NewAssetObject->PostLoad();
                Assets.Add(Guid, NewAssetObject);
                return { static_cast<T*>(NewAssetObject) };
            }
            
            //Failed to load the asset
            return nullptr;
		}

		template<typename T>
		AssetPtr<T> LoadAssetByGuid(const FGuid& InGuid)
		{
			return IsValidAsset(InGuid) ? LoadAssetByPath<T>(GetPath(InGuid)) : nullptr;
		}

		AssetObject* FindLoadedAsset(const FGuid& Id)
		{
			return Assets.Contains(Id) ? Assets[Id] : nullptr;
		}
        AssetObject* FindLoadedAsset(const FString& InPath)
        {
			if (!TryGetGuid(InPath))
			{
				return nullptr;
			}
            return FindLoadedAsset(GetGuid(InPath));
        }

        FGuid ReadAssetGuidInDisk(const FString& InPath);
		void UpdateGuidToPath(const FString& InPath);
        void RemoveGuidToPath(const FString& InPath);

		FString GetPath(const FGuid& InGuid) const;
		FGuid GetGuid(const FString& InPath) const;
		TOptional<FGuid> TryGetGuid(const FString& InPath) const;

		//The asset may be deleted outside
		bool IsValidAsset(const FGuid& Id) const { return GuidToPath.Contains(Id); }
		bool IsValidAsset(const FString& InPath) const { return GuidToPath.FindKey(InPath) != nullptr; }

        void Clear();
        void DestroyAsset(const FString& InAssetPath);

		void AddAssetThumbnail(const FGuid& InGuid, TRefCountPtr<GpuTexture> InThumbnail);
		GpuTexture* FindAssetThumbnail(const FGuid& InGuid) const;

        void RemoveAsset(AssetObject* InAsset);
		TArray<FString> GetManageredExts() const;

	private:
		TMap<FGuid, AssetPath> GuidToPath;
		TMap<FGuid, AssetObject*> Assets; //Loaded asset
		TMap<FGuid, TRefCountPtr<GpuTexture>> AssetThumbnailPool;
	};

    template<typename T>
    FArchive& operator<<(FArchive& Ar, AssetPtr<T>& InOutAssetPtr)
    {
        bool IsValid = !!InOutAssetPtr;
        Ar << IsValid;
        if (Ar.IsLoading())
        {
            if (IsValid)
            {
                FGuid AssetGuid;
                Ar << AssetGuid;
                //Maybe return a null AssetPtr because of outside deleting.
                InOutAssetPtr = TSingleton<AssetManager>::Get().LoadAssetByGuid<T>(AssetGuid);
            }
        }
        else
        {
            if (IsValid)
            {
                FGuid AssetGuid = InOutAssetPtr->GetGuid();;
                Ar << AssetGuid;
            }
        }
        return Ar;
    }
}
