#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class MaterialOp : public ShAssetOp
	{
		REFLECTION_TYPE(MaterialOp)
	public:
		MaterialOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
	};
}
