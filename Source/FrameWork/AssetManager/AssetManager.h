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

	class AssetManager
	{
	public:
	
		/*	template<typename T>
			AssetPtr<T> LoadAssetByPath(const FString& AssetPath)
			{
				return AssetImporter::Import<T>(AssetPath);
			}

			template<typename T>
			AssetPtr<T> LoadAssetByGuid()
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

//Resolve circular dependency
#include "AssetPtr.hpp"