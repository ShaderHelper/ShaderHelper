#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"
#include "AssetObject/StShader.h"
#include "AssetManager/AssetManager.h"
#include "Editor/PreviewViewPort.h"

namespace SH
{
    class ShaderToyPassNodeOp : public FW::ShObjectOp
    {
        REFLECTION_TYPE(ShaderToyPassNodeOp)
    public:
        ShaderToyPassNodeOp() = default;
        
        FW::MetaType* SupportType() override;
        void OnSelect(FW::ShObject* InObject) override;
    };

	class ShaderToyPassNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		ShaderToyPassNode();

	public:
		void Serialize(FArchive& Ar) override;
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		TArray<FW::GraphPin*> GetPins() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.27f, 0.13f, 0.0f }; }
		void Exec(FW::GraphExecContext& Context) override;

		void SetPassShader(FW::AssetPtr<StShader> InPassShader);

	public:
		ChannelPin Slot0{ FText::FromString("iChannel0"), FW::PinDirection::Input };
        ChannelPin Slot1{ FText::FromString("iChannel1"), FW::PinDirection::Input };
        ChannelPin Slot2{ FText::FromString("iChannel2"), FW::PinDirection::Input };
        ChannelPin Slot3{ FText::FromString("iChannel3"), FW::PinDirection::Input };
		GpuTexturePin PassOutput{ FText::FromString("Texture"), FW::PinDirection::Output };

	private:
		TSharedPtr<FW::PreviewViewPort> Preview;
		FW::AssetPtr<StShader> PassShader;
		TRefCountPtr<FW::GpuShader> VertexShader, PixelShader;
	};
}
