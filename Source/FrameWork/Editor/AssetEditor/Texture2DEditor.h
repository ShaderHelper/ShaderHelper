#pragma once
#include "AssetEditor.h"

namespace FRAMEWORK
{
	class Texture2DOp : public AssetOp
	{
	public:
		Texture2DOp() = default;

        struct MetaType* SupportAsset() override;
		void OnOpen(const FString& InAssetPath) override;
	};
}
