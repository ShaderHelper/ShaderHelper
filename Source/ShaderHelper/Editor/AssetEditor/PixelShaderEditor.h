#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class PixelShaderOp : public ShAssetOp
	{
		REFLECTION_TYPE(PixelShaderOp)
	public:
		PixelShaderOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
		void OnRename(const FString& OldPath, const FString& NewPath) override;
		void OnMove(const FString& OldPath, const FString& NewPath) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}
