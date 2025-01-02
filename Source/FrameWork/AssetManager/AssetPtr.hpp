#pragma once

namespace FRAMEWORK
{
	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>::AssetPtr(std::nullptr_t)
		: Asset(nullptr)
	{

	}

	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>::AssetPtr(T* InAsset)
		: Asset(InAsset)
	{
		if (OwnerShip != AssetOwnerShip::Assign)
		{
			if (Asset) {
				TSingleton<AssetManager>::Get().AddRef(Asset);
			}
		}
	}

	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>::AssetPtr(const AssetPtr& InAssetPtr)
		: Asset(InAssetPtr.Asset)
	{
		if (OwnerShip != AssetOwnerShip::Assign)
		{
			if (Asset) {
				TSingleton<AssetManager>::Get().AddRef(Asset);
			}
		}
	}

	template<typename T, AssetOwnerShip OwnerShip>
	template<typename OtherType, AssetOwnerShip OtherOwnerShip, typename>
	AssetPtr<T, OwnerShip>::AssetPtr(const AssetPtr<OtherType, OtherOwnerShip>& OtherAssetPtr)
		: Asset(OtherAssetPtr.Asset)
	{
		if (OwnerShip != AssetOwnerShip::Assign)
		{
			if (Asset) {
				TSingleton<AssetManager>::Get().AddRef(Asset);
			}
		}
	}

	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>& AssetPtr<T, OwnerShip>::operator=(const AssetPtr& OtherAssetPtr)
	{
		AssetPtr Temp{ OtherAssetPtr };
		Swap(Temp, *this);
		return *this;
	}

	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>::~AssetPtr()
	{
		if (OwnerShip != AssetOwnerShip::Assign)
		{
			if (IsValid()) {
				TSingleton<AssetManager>::Get().ReleaseRef(Asset);
			}
		}
	}

	template<typename T, AssetOwnerShip OwnerShip>
	T* AssetPtr<T, OwnerShip>::operator->() const
	{
		return Asset;
	}

	template<typename T, AssetOwnerShip OwnerShip>
	T* AssetPtr<T, OwnerShip>::Get() const
	{
		return Asset;
	}

	template<typename T, AssetOwnerShip OwnerShip>
	FGuid AssetPtr<T, OwnerShip>::GetGuid() const
	{
		return Asset->GetGuid();
	}

	template<typename T, AssetOwnerShip OwnerShip>
	bool AssetPtr<T, OwnerShip>::IsValid() const
	{
		bool bNull = (Asset == nullptr);
		bool bLoaded = TSingleton<AssetManager>::Get().IsLoadedAsset(Asset);
		return (!bNull) && bLoaded;
	}

	template<typename T, AssetOwnerShip OwnerShip>
	AssetPtr<T, OwnerShip>::operator bool() const
    {
        return IsValid();
    }

    template<typename T1, typename T2, AssetOwnerShip OwnerShip1, AssetOwnerShip OwnerShip2>
    bool operator==(const AssetPtr<T1, OwnerShip1>& Lhs, const AssetPtr<T2, OwnerShip2>& Rhs)
    {
		AssetObject* LhsAsset = Lhs.Get();
		AssetObject* RhsAsset = Rhs.Get();
		if (!LhsAsset || !RhsAsset)
		{
			return LhsAsset == RhsAsset;
		}
		else
		{
			return Lhs.GetGuid() == Rhs.GetGuid();
		}
        
    }
    
	template<typename T, AssetOwnerShip OwnerShip>
    uint32 GetTypeHash(const AssetPtr<T, OwnerShip>& InAssetPtr)
    {
        return GetTypeHash(InAssetPtr->GetGuid());
    }

	template<typename T, AssetOwnerShip OwnerShip>
	FArchive& operator<<(FArchive& Ar, AssetPtr<T, OwnerShip>& InOutAssetPtr)
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
