#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class StShaderOp : public FW::AssetOp
	{
		REFLECTION_TYPE(StShaderOp)
	public:
		StShaderOp() = default;

        FW::MetaType* SupportType() override;
        void OnSelect(FW::ShObject* InObject) override;
		void OnOpen(const FString& InAssetPath) override;
       
        void OnRename(const FString& OldPath, const FString& NewPath) override;
        void OnMove(const FString& OldPath, const FString& NewPath) override;
        
        void OnAdd(const FString& InAssetPath) override;
        void OnDelete(const FString& InAssetPath) override;
	};
}
