#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class ShaderToyOp : public ShAssetOp
	{
		REFLECTION_TYPE(ShaderToyOp)
	public:
		ShaderToyOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
		bool OnCreate(FW::AssetObject* InAsset) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}
