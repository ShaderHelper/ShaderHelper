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
		AssetPtr(T* InAsset, const FGuid& InGuid);

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
        
        explicit operator bool() const;

	private:
		T* Asset;
		FGuid Guid;
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

			FGuid Guid = GetGuid(InAssetPath);
			if (Assets.Contains(Guid))
			{
				return { static_cast<T*>(Assets[Guid]), Guid };
			}

			TArray<ShReflectToy::MetaType*> AssetObjectMetaTypes = ShReflectToy::GetMetaTypes<AssetObject>();
			FString AssetExt = FPaths::GetExtension(InAssetPath);
			AssetObject* NewAssetObject = nullptr;
			for (auto MetaTypePtr : AssetObjectMetaTypes)
			{
				AssetObject* DefaultAssetObject = static_cast<AssetObject*>(MetaTypePtr->GetDefaultObject());
				if (DefaultAssetObject && DefaultAssetObject->FileExtension().Contains(AssetExt))
				{
					NewAssetObject = static_cast<AssetObject*>(MetaTypePtr->Construct());
					break;
				}
			}
			check(NewAssetObject);
			TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InAssetPath));
            if(Ar)
            {
                NewAssetObject->Serialize(*Ar);
                NewAssetObject->PostLoad();
                Assets.Add(Guid, NewAssetObject);
                return { static_cast<T*>(NewAssetObject), Guid };
            }
            
            //Failed to load the asset
            return nullptr;
		}

		template<typename T>
		AssetPtr<T> LoadAssetByGuid(const FGuid& InGuid)
		{
			return LoadAssetByPath<T>(GetPath(InGuid));
		}

		void UpdateGuidToPath(const FGuid& InGuid, const FString& InPath);

		FString GetPath(const FGuid& InGuid) const;
		FGuid GetGuid(const FString& InPath) const;

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
		if (Ar.IsLoading())
		{
			FGuid AssetGuid;
			Ar << AssetGuid;
			InOutAssetPtr = TSingleton<AssetManager>::Get().LoadAssetByGuid<T>(AssetGuid);
		}
		else
		{
			Ar << InOutAssetPtr->GetGuid();
		}
		return Ar;
	}

}

//Resolve circular dependency
#include "AssetPtr.hpp"
