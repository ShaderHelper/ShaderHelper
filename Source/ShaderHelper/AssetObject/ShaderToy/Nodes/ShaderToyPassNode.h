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
		FText GetNodeTitle() const override { return FText::FromString("RenderPass"); }
		TArray<FRAMEWORK::GraphPin*> GetPins() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 1.0f, 0.6f, 0.2f }; }
		void Exec() override;

		ChannelPin Slot0{ FText::FromString("iChannel0"), FRAMEWORK::PinDirection::Input };
		ChannelPin Slot1{ FText::FromString("iChannel1"), FRAMEWORK::PinDirection::Input };
		ChannelPin Slot2{ FText::FromString("iChannel2"), FRAMEWORK::PinDirection::Input };
		ChannelPin Slot3{ FText::FromString("iChannel3"), FRAMEWORK::PinDirection::Input };
		GpuTexturePin PassOutput{ FText::FromString("Texture"), FRAMEWORK::PinDirection::Output };
	};
}