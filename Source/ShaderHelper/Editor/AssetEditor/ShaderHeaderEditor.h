#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class ShaderHeaderOp : public ShAssetOp
	{
		REFLECTION_TYPE(ShaderHeaderOp)
	public:
		ShaderHeaderOp() = default;

        FW::MetaType* SupportType() override;
		void OnSelect(FW::ShObject* InObject) override;
		void OnOpen(const FString& InAssetPath) override;
	};
}
