#pragma once
#include "AssetObject/AssetObject.h"

namespace FRAMEWORK
{
	class AssetOp
	{
	public:
		AssetOp() = default;
		virtual ~AssetOp() = default;

	public:
        virtual struct ShReflectToy::MetaType* SupportAsset() = 0;
		virtual void Open(const FString& InAssetPath) = 0;
	};
}
