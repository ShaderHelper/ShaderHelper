#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"

namespace SH
{
	class ShaderToyOuputNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyOuputNode)
	public:
		ShaderToyOuputNode();

	public:
		void Serialize(FArchive& Ar) override;
		TArray<FW::GraphPin*> GetPins() override;
		void Exec(FW::GraphExecContext& Context) override;

		GpuTexturePin ResultPin{ FText::FromString("Texture"), FW::PinDirection::Input };
	};
}