#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShaderPassOp : public FRAMEWORK::AssetOp
	{
	public:
		ShaderPassOp() = default;

        struct FRAMEWORK::ShReflectToy::MetaType* SupportAsset() override;
		void Open(const FString& InAssetPath) override;
	};
}
