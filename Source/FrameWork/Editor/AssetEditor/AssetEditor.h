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
        virtual void OnRename(const FString& OldPath, const FString& NewPath);
        virtual void OnMove(const FString& OldPath, const FString& NewPath) {}
        virtual void OnOpen(const FString& InAssetPath) {}

        virtual void OnAdd(const FString& InAssetPath) {}
		virtual void OnDelete(const FString& InAssetPath);

		//For non-imported resources
		virtual void OnCreate(AssetObject* InAsset) {};
	};
    
    AssetOp* GetAssetOp(const FString& InAssetPath);
	AssetOp* GetAssetOp(MetaType* InAssetMetaType);
}
