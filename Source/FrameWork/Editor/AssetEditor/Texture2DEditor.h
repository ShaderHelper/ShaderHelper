#pragma once
#include "AssetEditor.h"

namespace FRAMEWORK
{
	class Texture2DOp : public AssetOp
	{
	public:
		Texture2DOp() = default;

		void Open(AssetObject* InObject) override;
	};
}
