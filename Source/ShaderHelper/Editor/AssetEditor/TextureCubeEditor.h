#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class TextureCubeOp : public ShAssetOp
	{
		REFLECTION_TYPE(TextureCubeOp)
	public:
		TextureCubeOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
		bool OnCreate(FW::AssetObject* InAsset) override;
	};
}
