#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"

namespace SH
{
	class ShaderToyPassNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		void Serialize(FArchive& Ar) override;
		FText GetNodeTitle() const override { return FText::FromString("RenderPass"); }
		TArray<FW::GraphPin*> GetPins() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 1.0f, 0.6f, 0.2f }; }
		void Exec(FW::GraphExecContext& Context) override;

		ChannelPin Slot0{ FText::FromString("iChannel0"), FW::PinDirection::Input };
		GpuTexturePin PassOutput{ FText::FromString("Texture"), FW::PinDirection::Output };
	};
}