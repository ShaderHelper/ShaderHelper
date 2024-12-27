#pragma once
#include "AssetObject/AssetObject.h"
#include "Common/Util/Reflection.h"

namespace FRAMEWORK
{
	template<typename T>
	class AssetPtr
	{
		friend class AssetManager;

		template <typename OtherType>
		friend class AssetPtr;
	private:
		AssetPtr(T* InAsset);

	public:
		AssetPtr(std::nullptr_t = nullptr);
		AssetPtr(const AssetPtr& InAssetPtr);

		template<typename OtherType,
			typename = decltype(ImplicitConv<T*>((OtherType*)nullptr))
		>
		AssetPtr(const AssetPtr<OtherType>& OtherAssetPtr);

		AssetPtr& operator=(const AssetPtr& OtherAssetPtr);

		~AssetPtr();

		T* operator->() const;

		T* Get() const;
		FGuid GetGuid() const;

		bool IsValid() const;
        explicit operator bool() const;

	private:
		T* Asset;
	};

	class GpuTexture;

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

		bool IsLoadedAsset(AssetObject* InAsset) const
		{
			return AssetRefCounts.Contains(InAsset);
		}

		void UpdateGuidToPath(const FString& InPath);
        void RemoveGuidToPath(const FString& InPath);

		FString GetPath(const FGuid& InGuid) const;
		FGuid GetGuid(const FString& InPath) const;

		//The asset may be deleted outside
		bool IsValidAsset(const FGuid& Id) const { return GuidToPath.Contains(Id); }
		bool IsValidAsset(const FString& InPath) const { return GuidToPath.FindKey(InPath) != nullptr; }

		void AddAssetThumbnail(const FGuid& InGuid, TRefCountPtr<GpuTexture> InThumbnail);
		GpuTexture* FindAssetThumbnail(const FGuid& InGuid) const;

		void Clear();
		TArray<FString> GetManageredExts() const;
		
		void AddRef(AssetObject* InAssetObject);
		void ReleaseRef(AssetObject* InAssetObject);


	private:
		TMap<FGuid, FString> GuidToPath;
		TMap<FGuid, AssetObject*> Assets;
		TMap<AssetObject*, uint32> AssetRefCounts;
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

//Resolve circular dependency
#include "AssetPtr.hpp"
