#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"

namespace SH
{
	class ShaderToyOuputNode : public FRAMEWORK::GraphNode
	{
		REFLECTION_TYPE(ShaderToyOuputNode)
	public:
		void Serialize(FArchive& Ar) override;
		FText GetNodeTitle() const override { return FText::FromString("Output"); }
		TArray<FRAMEWORK::GraphPin*> GetPins() override;
		void Exec() override;

		GpuTexturePin ResultPin{ FText::FromString("Texture"), FRAMEWORK::PinDirection::Input };
	};
}