#pragma once
#include "AssetObject/Graph.h"

namespace SH
{
	class ShaderToyPassNode : public FRAMEWORK::GraphNode
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		void Serialize(FArchive& Ar) override;
		FText GetNodeTitle() const override { return LOCALIZATION("RenderPass"); }
		TArray<FRAMEWORK::GraphPin*> GetPins() override;
		void Exec();
	};
}