#pragma once
#include "AssetObject.h"

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
		AssetPtr(const AssetPtr& InAssetRef);

		template<typename OtherType,
			typename = decltype(ImplicitConv<T*>((OtherType*)nullptr))
		>
		AssetPtr(const AssetPtr<OtherType>& OtherAssetRef);

		AssetPtr& operator=(const AssetPtr& OtherAssetRef);

		~AssetPtr();

		T* operator->() const;

		T* Get() const;

	private:
		T* Asset;
	};

	class GpuTexture;

	class AssetManager
	{
	public:
		/*	
			template<typename T>
			AssetPtr<T> LoadAssetByPath(const FString& InAssetPath)
			{

			}

			template<typename T>
			AssetPtr<T> LoadAssetByGuid()
			{
				
			}

			FString GetPath(guid) const;
		*/

		//void AddThumbnail(AssetObject* InAssetObject, GpuTexture* InThumbnail);
		void AddRef(AssetObject* InAssetObject);
		void ReleaseRef(AssetObject* InAssetObject);

	private:
		//TMap<, FString> GuidToPath;
		//TMap<, AssetObject*> Asssets;
		//TMap<AssetObject*, uint32> AssetRefCounts;
		//TMap<AssetObject*, GpuTexture*> AssetThumbnailPool;
	};

}

//Resolve circular dependency
#include "AssetPtr.hpp"