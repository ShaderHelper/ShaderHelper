#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class RenderOp : public ShAssetOp
	{
		REFLECTION_TYPE(RenderOp)
	public:
		RenderOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
		bool OnCreate(FW::AssetObject* InAsset) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}
