#pragma once
#include "AssetObject/Graph.h"

namespace SH
{
	class ShaderToy : public FRAMEWORK::Graph
	{
		REFLECTION_TYPE(ShaderToy)
	public:
		ShaderToy();

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;

	};
}