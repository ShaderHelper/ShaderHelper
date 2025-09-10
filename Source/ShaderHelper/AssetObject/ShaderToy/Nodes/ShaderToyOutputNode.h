#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/ShaderToy.h"

namespace SH
{

    class ShaderToyOutputNodeOp : public FW::ShObjectOp
    {
        REFLECTION_TYPE(ShaderToyOutputNodeOp)
    public:
        ShaderToyOutputNodeOp() = default;
        
        FW::MetaType* SupportType() override;
        void OnSelect(FW::ShObject* InObject) override;
    };

	class ShaderToyOutputNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyOutputNode)
	public:
		ShaderToyOutputNode();
        ~ShaderToyOutputNode();
        
	public:
        void InitPins() override;
		void Serialize(FArchive& Ar) override;
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		
		ShaderToyFormat Format;
	};
}
