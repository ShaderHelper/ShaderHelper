#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyOuputNode>("Present Node")
		.BaseClass<GraphNode>()
	)
    REFLECTION_REGISTER(AddClass<ShaderToyOuputNodeOp>()
        .BaseClass<ShObjectOp>()
    )

    MetaType* ShaderToyOuputNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyOuputNode>();
    }

    void ShaderToyOuputNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	ShaderToyOuputNode::ShaderToyOuputNode()
	{
		ObjectName = FText::FromString("Present");
	}

    ShaderToyOuputNode::~ShaderToyOuputNode()
    {

    }

	void ShaderToyOuputNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);

		ResultPin.Serialize(Ar);
	}

	TArray<GraphPin*> ShaderToyOuputNode::GetPins()
	{
		return { &ResultPin };
	}

	bool ShaderToyOuputNode::Exec(GraphExecContext& Context)
	{
        return true;
	}
}
