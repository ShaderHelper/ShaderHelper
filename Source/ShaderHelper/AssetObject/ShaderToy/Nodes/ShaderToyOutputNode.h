#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"

namespace SH
{

    class ShaderToyOuputNodeOp : public FW::ShObjectOp
    {
        REFLECTION_TYPE(ShaderToyOuputNodeOp)
    public:
        ShaderToyOuputNodeOp() = default;
        
        FW::MetaType* SupportType() override;
        void OnSelect(FW::ShObject* InObject) override;
    };

	class ShaderToyOuputNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyOuputNode)
	public:
		ShaderToyOuputNode();
        ~ShaderToyOuputNode();
        
	public:
		void Serialize(FArchive& Ar) override;
		TArray<FW::GraphPin*> GetPins() override;
		void Exec(FW::GraphExecContext& Context) override;

		GpuTexturePin ResultPin{ FText::FromString("Texture"), FW::PinDirection::Input };
	};
}
