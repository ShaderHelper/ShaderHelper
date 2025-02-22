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
        void InitPins() override;
		void Serialize(FArchive& Ar) override;
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;
	};
}
