#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class Texture2dOp : public FW::AssetOp
	{
		REFLECTION_TYPE(Texture2dOp)
	public:
		Texture2dOp() = default;

		FW::MetaType* SupportType() override;
	};
}
