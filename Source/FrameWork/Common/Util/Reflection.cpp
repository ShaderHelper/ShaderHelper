#include "CommonHeader.h"
#include "Reflection.h"
#include "AssetObject/AssetObject.h"

namespace FW
{
	
	TMap<FString, MetaType*>& GetTypeNameToMetaType()
	{
		static TMap<FString, MetaType*> TypeNameToMetaType;
		return TypeNameToMetaType;
	}

	TMap<FString, MetaType*>& GetRegisteredNameToMetaType()
	{
		static TMap<FString, MetaType*> RegisteredNameToMetaType;
		return RegisteredNameToMetaType;
	}

    bool MetaMemberData::IsAssetRef() const
    {
		MetaType* MemberMetaType = GetMetaType();
		return MemberMetaType ? MemberMetaType->IsDerivedFrom<AssetObject>() : false;
    }

}
