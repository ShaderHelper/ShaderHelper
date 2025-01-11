#pragma once
#include "AssetObject/AssetObject.h"

namespace FW
{
	class FRAMEWORK_API AssetOp : public ShObjectOp
	{
		REFLECTION_TYPE(AssetOp)
	public:
		AssetOp() = default;
		//Manually trigger OnOpen
		static void OpenAsset(AssetObject* InAsset);

	public:
        virtual void OnOpen(const FString& InAssetPath) {}
		//OnDelete and OnAdd will be triggered one after the other if rename or move the asset.
        virtual void OnAdd(const FString& InAssetPath) {}
		virtual void OnDelete(const FString& InAssetPath);
		//
		virtual void OnCreate(AssetObject* InAsset) {};
	};
    
    AssetOp* GetAssetOp(const FString& InAssetPath);
	AssetOp* GetAssetOp(MetaType* InAssetMetaType);
}
