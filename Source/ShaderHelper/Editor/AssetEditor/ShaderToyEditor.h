#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShaderToyOp : public FW::AssetOp
	{
		REFLECTION_TYPE(ShaderToyOp)
	public:
		ShaderToyOp() = default;

		struct FW::MetaType* SupportAsset() override;
		void OnOpen(const FString& InAssetPath) override;
		void OnCreate(FW::AssetObject* InAsset) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}