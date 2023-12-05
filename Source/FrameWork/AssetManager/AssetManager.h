#pragma once
#include "AssetObject.h"

namespace FRAMEWORK
{
	class AssetManager;

	template<typename T>
	class AssetRef
	{
		friend AssetManager;

		template <typename OtherType>
		friend class AssetRef;
	private:
		AssetRef(T& InAsset)
			: Asset(&InAsset)
		{
			TSingleton<AssetManager>::Get().AddRef(Asset);
		}

	public:
		AssetRef(const AssetRef& InAssetRef)
			: Asset(InAssetRef.Asset)
		{
			TSingleton<AssetManager>::Get().AddRef(Asset);
		}

		template<typename OtherType,
			typename = decltype(ImplicitConv<T*>((OtherType*)nullptr))
		>
		AssetRef(const AssetRef<OtherType>& OtherAssetRef)
			: Asset(OtherAssetRef.Asset)
		{
			TSingleton<AssetManager>::Get().AddRef(Asset);
		}

		AssetRef& operator=(const AssetRef& OtherAssetRef)
		{
			AssetRef Temp{ OtherAssetRef };
			Swap(Temp, *this);
			return *this;
		}

		~AssetRef()
		{
			TSingleton<AssetManager>::Get().ReleaseRef(Asset);
		}

		T* operator->() const
		{
			check(Asset != nullptr);
			return Asset;
		}

		T& Get() const
		{
			check(Asset != nullptr);
			return *Asset;
		}

	private:
		T* Asset;
	};

	class AssetManager
	{
	public:
	
		/*	template<typename T>
			AssetRef<T> LoadAssetByPath(const FString& AssetPath)
			{
				return AssetImporter::Import<T>(AssetPath);
			}

			template<typename T>
			AssetRef<T> LoadAssetByGuid()
			{
				return AssetImporter::Import<T>();
			}*/

		FString GuidToAssetPath();
		void AddRef(AssetObject* InAssetObject);
		void ReleaseRef(AssetObject* InAssetObject);

	private:
		//TMap<, AssetObject*> Asssets;
	};
}