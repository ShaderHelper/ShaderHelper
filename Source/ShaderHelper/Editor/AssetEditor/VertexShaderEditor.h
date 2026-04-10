#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class VertexShaderOp : public ShAssetOp
	{
		REFLECTION_TYPE(VertexShaderOp)
	public:
		VertexShaderOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
		void OnRename(const FString& OldPath, const FString& NewPath) override;
		void OnMove(const FString& OldPath, const FString& NewPath) override;
		void OnDelete(const FString& InAssetPath) override;
	};
}
