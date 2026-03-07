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
		bool OnCreate(FW::AssetObject* InAsset) override;
	};
}
