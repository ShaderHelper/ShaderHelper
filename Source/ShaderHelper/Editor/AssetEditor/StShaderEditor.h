#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class StShaderOp : public FRAMEWORK::AssetOp
	{
		REFLECTION_TYPE(StShaderOp)
	public:
		StShaderOp() = default;

        struct FRAMEWORK::MetaType* SupportAsset() override;
		void OnOpen(const FString& InAssetPath) override;
       
        void OnAdd(const FString& InAssetPath) override;
        void OnDelete(const FString& InAssetPath) override;
	};
}
