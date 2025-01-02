#pragma once
#include "AssetObject/Graph.h"

namespace SH
{

	struct ShaderToyExecContext : FW::GraphExecContext
	{

	};

	class ShaderToy : public FW::Graph
	{
		REFLECTION_TYPE(ShaderToy)
	public:
		ShaderToy() = default;

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		TArray<FW::MetaType*> SupportNodes() const override;
	};
}