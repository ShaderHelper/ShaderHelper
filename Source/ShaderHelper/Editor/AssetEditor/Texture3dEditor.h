#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class Texture3dOp : public ShAssetOp
	{
		REFLECTION_TYPE(Texture3dOp)
	public:
		Texture3dOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
	};
}
