#pragma once

namespace FRAMEWORK
{
	template<typename T>
	AssetPtr<T>::AssetPtr(std::nullptr_t)
		: Asset(nullptr)
	{

	}

	template<typename T>
	AssetPtr<T>::AssetPtr(T* InAsset, const FGuid& InGuid)
		: Asset(InAsset)
		, Guid(InGuid)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	AssetPtr<T>::AssetPtr(const AssetPtr& InAssetPtr)
		: Asset(InAssetPtr.Asset)
		, Guid(InAssetPtr.Guid)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	template<typename OtherType, typename>
	AssetPtr<T>::AssetPtr(const AssetPtr<OtherType>& OtherAssetPtr)
		: Asset(OtherAssetPtr.Asset)
		, Guid(OtherAssetPtr.Guid)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	AssetPtr<T>& AssetPtr<T>::operator=(const AssetPtr& OtherAssetPtr)
	{
		AssetPtr Temp{ OtherAssetPtr };
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

	template<typename T>
	FGuid AssetPtr<T>::GetGuid() const
	{
		return Guid;
	}
}