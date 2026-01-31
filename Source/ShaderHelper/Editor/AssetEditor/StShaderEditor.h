#pragma once
#include "ShAssetEditor.h"

namespace SH
{
	class StShaderOp : public ShAssetOp
	{
		REFLECTION_TYPE(StShaderOp)
	public:
		StShaderOp() = default;

        FW::MetaType* SupportType() override;
		void OnOpen(const FString& InAssetPath) override;
       
        void OnRename(const FString& OldPath, const FString& NewPath) override;
        void OnMove(const FString& OldPath, const FString& NewPath) override;
        
        void OnAdd(const FString& InAssetPath) override;
        void OnDelete(const FString& InAssetPath) override;
	};
}
