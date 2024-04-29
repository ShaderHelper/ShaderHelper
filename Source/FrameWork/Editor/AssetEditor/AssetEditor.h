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
		virtual void Open(AssetObject* InObject) = 0;
	};
}
