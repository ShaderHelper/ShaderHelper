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
		void OnSelect(FW::ShObject* InObject) override;
		void OnOpen(const FString& InAssetPath) override;
		void OnCreate(FW::AssetObject* InAsset) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}
