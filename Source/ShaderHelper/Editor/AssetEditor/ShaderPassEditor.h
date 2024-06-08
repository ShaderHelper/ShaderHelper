#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShaderPassOp : public FRAMEWORK::AssetOp
	{
	public:
		ShaderPassOp() = default;

        struct FRAMEWORK::MetaType* SupportAsset() override;
		void OnOpen(const FString& InAssetPath) override;
        
        //OnDelete and OnAdd will be triggered one after the other if rename or move the asset.
        void OnAdd(const FString& InAssetPath) override;
        void OnDelete(const FString& InAssetPath) override;
	};
}
