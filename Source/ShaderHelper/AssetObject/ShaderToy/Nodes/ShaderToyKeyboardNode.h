#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/ShaderToy.h"

namespace SH
{

    class ShaderToyKeyboardNodeOp : public ShPropertyOp
    {
        REFLECTION_TYPE(ShaderToyKeyboardNodeOp)
    public:
		ShaderToyKeyboardNodeOp() = default;
        
        FW::MetaType* SupportType() override;
    };

	class ShaderToyKeyboardNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyKeyboardNode)
	public:
		ShaderToyKeyboardNode();
        ~ShaderToyKeyboardNode();
		void Init() override;
        
	public:
        void InitPins();
		void Serialize(FArchive& Ar) override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.2f, 0.2f, 0.4f }; }
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;

		FW::GpuTextureFormat Format;
		uint32 Width{};
		uint32 Height{};
	};
}
