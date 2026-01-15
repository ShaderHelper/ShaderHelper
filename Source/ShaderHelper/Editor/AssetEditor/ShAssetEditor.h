#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShAssetOp : public FW::AssetOp
	{
		REFLECTION_TYPE(ShAssetOp)
	public:
		ShAssetOp() = default;

		void OnNavigate(const FString& InAssetPath) override;
	};
}
