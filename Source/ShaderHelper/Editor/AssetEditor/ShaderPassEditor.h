#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShaderPassOp : public FRAMEWORK::AssetOp
	{
	public:
		ShaderPassOp() = default;

		void Open(FRAMEWORK::AssetObject* InObject) override;
	};
}
