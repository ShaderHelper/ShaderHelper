#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShaderToyOp : public FRAMEWORK::AssetOp
	{
		REFLECTION_TYPE(ShaderToyOp)
	public:
		ShaderToyOp() = default;

		struct FRAMEWORK::MetaType* SupportAsset() override;
		void OnOpen(const FString& InAssetPath) override;
		void OnCreate(FRAMEWORK::AssetObject* InAsset) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}