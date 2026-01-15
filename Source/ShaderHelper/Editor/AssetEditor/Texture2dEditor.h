#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class Texture2dOp : public ShAssetOp
	{
		REFLECTION_TYPE(Texture2dOp)
	public:
		Texture2dOp() = default;

		FW::MetaType* SupportType() override;
	};
}
