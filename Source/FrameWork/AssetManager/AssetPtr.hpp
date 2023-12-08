#pragma once

namespace FRAMEWORK
{
	template<typename T>
	AssetPtr<T>::AssetPtr(T* InAsset)
		: Asset(InAsset)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	AssetPtr<T>::AssetPtr(const AssetPtr& InAssetRef)
		: Asset(InAssetRef.Asset)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	template<typename OtherType, typename>
	AssetPtr<T>::AssetPtr(const AssetPtr<OtherType>& OtherAssetRef)
		: Asset(OtherAssetRef.Asset)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	AssetPtr<T>& AssetPtr<T>::operator=(const AssetPtr& OtherAssetRef)
	{
		AssetPtr Temp{ OtherAssetRef };
		Swap(Temp, *this);
		return *this;
	}

	template<typename T>
	AssetPtr<T>::~AssetPtr()
	{
		TSingleton<AssetManager>::Get().ReleaseRef(Asset);
	}

	template<typename T>
	T* AssetPtr<T>::operator->() const
	{
		return Asset;
	}

	template<typename T>
	T* AssetPtr<T>::Get() const
	{
		return Asset;
	}
}