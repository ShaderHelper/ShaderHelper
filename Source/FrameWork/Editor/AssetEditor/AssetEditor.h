#pragma once
#include "AssetObject/AssetObject.h"

namespace FRAMEWORK
{
	class AssetOp
	{
		REFLECTION_TYPE(AssetOp)
	public:
		AssetOp() = default;
		virtual ~AssetOp() = default;

	public:
        virtual struct MetaType* SupportAsset() = 0;
        virtual void OnOpen(const FString& InAssetPath) {}
        virtual void OnAdd(const FString& InAssetPath) {}
        virtual void OnDelete(const FString& InAssetPath) {}
		virtual void OnCreate(AssetObject* InAsset) {};
	};
    
    AssetOp* GetAssetOp(const FString& InAssetPath);
	AssetOp* GetAssetOp(MetaType* InAssetMetaType);
}
