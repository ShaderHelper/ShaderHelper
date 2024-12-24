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
		FText GetNodeTitle() const override { return LOCALIZATION("Output"); }
		TArray<FRAMEWORK::GraphPin*> GetPins() override;
		void Exec();

		GpuTexturePin ResultPin{ LOCALIZATION("Result"), FRAMEWORK::PinDirection::Input };
	};
}