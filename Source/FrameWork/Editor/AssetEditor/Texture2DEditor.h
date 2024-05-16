#pragma once
#include "AssetEditor.h"

namespace FRAMEWORK
{
	class Texture2DOp : public AssetOp
	{
	public:
		Texture2DOp() = default;

        struct ShReflectToy::MetaType* SupportAsset() override;
		void Open(const FString& InAssetPath) override;
	};
}
