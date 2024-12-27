#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"

namespace SH
{
	class ShaderToyPassNode : public FRAMEWORK::GraphNode
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		void Serialize(FArchive& Ar) override;
		FText GetNodeTitle() const override { return LOCALIZATION("RenderPass"); }
		TArray<FRAMEWORK::GraphPin*> GetPins() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 1.0f, 0.6f, 0.2f }; }
		void Exec() override;

		SlotPin Slot1{ LOCALIZATION("Slot1"), FRAMEWORK::PinDirection::Input };
		SlotPin Slot2{ LOCALIZATION("Slot2"), FRAMEWORK::PinDirection::Input };
		SlotPin Slot3{ LOCALIZATION("Slot3"), FRAMEWORK::PinDirection::Input };
		SlotPin Slot4{ LOCALIZATION("Slot4"), FRAMEWORK::PinDirection::Input };
		GpuTexturePin PassOutput{ LOCALIZATION("Texture"), FRAMEWORK::PinDirection::Output };
	};
}