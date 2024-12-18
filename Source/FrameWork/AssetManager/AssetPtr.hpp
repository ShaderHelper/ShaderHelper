#pragma once

namespace FRAMEWORK
{
	template<typename T>
	AssetPtr<T>::AssetPtr(std::nullptr_t)
		: Asset(nullptr)
	{

	}

	template<typename T>
	AssetPtr<T>::AssetPtr(T* InAsset)
		: Asset(InAsset)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	AssetPtr<T>::AssetPtr(const AssetPtr& InAssetPtr)
		: Asset(InAssetPtr.Asset)
	{
		TSingleton<AssetManager>::Get().AddRef(Asset);
	}

	template<typename T>
	template<typename OtherType, typename>
	AssetPtr<T>::AssetPtr(const AssetPtr<OtherType>& OtherAssetPtr)
		: Asset(OtherAssetPtr.Asset)
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
		if (Asset) {
			TSingleton<AssetManager>::Get().ReleaseRef(Asset);
		}
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
		return Asset->GetGuid();
	}

    template<typename T>
    AssetPtr<T>::operator bool() const
    {
        return Asset != nullptr;
    }

    template<typename T1, typename T2>
    bool operator==(const AssetPtr<T1>& Lhs, const AssetPtr<T2>& Rhs)
    {
        return Lhs.GetGuid() == Rhs.GetGuid();
    }
    
    template<typename T>
    uint32 GetTypeHash(const AssetPtr<T>& InAssetPtr)
    {
        return GetTypeHash(InAssetPtr->GetGuid());
    }
    
}
