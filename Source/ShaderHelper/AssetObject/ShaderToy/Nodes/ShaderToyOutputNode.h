#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/ShaderToy.h"

namespace SH
{

    class ShaderToyOutputNodeOp : public ShPropertyOp
    {
        REFLECTION_TYPE(ShaderToyOutputNodeOp)
    public:
        ShaderToyOutputNodeOp() = default;
        
        FW::MetaType* SupportType() override;
		void OnCancelSelect(FW::ShObject* InObject) override;
        void OnSelect(FW::ShObject* InObject) override;
    };

	class ShaderToyOutputNode : public FW::GraphNode
	{
		REFLECTION_TYPE(ShaderToyOutputNode)
	public:
		ShaderToyOutputNode();
        ~ShaderToyOutputNode();
		void Init() override;
        
	public:
        void InitPins();
		void Serialize(FArchive& Ar) override;
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
		
		ShaderToyFormat Format;
		int32 Layer;
		float AreaFraction;
	};
}
