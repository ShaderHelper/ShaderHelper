#pragma once
#include "Editor/AssetEditor/AssetEditor.h"

namespace SH
{
	class ShPropertyOp : public FW::ShObjectOp
	{
		REFLECTION_TYPE(ShPropertyOp)
	public:
		ShPropertyOp() = default;

		void OnCancelSelect(FW::ShObject* InObject) override;
		void OnSelect(FW::ShObject* InObject) override;
	};

	class ShAssetOp : public FW::AssetOp
	{
		REFLECTION_TYPE(ShAssetOp)
	public:
		ShAssetOp() = default;

		void OnNavigate(const FString& InAssetPath) override;
		void OnCancelSelect(FW::ShObject* InObject) override;
		void OnSelect(FW::ShObject* InObject) override;
	};
}
