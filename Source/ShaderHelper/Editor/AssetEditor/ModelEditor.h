#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class ModelOp : public ShAssetOp
	{
		REFLECTION_TYPE(ModelOp)
	public:
		ModelOp() = default;

		FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
	};
}
