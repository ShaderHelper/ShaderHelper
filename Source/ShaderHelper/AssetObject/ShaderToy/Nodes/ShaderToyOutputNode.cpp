#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"

using namespace FW;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToyOuputNode>("Present Node")
		.BaseClass<GraphNode>()
	)

	ShaderToyOuputNode::ShaderToyOuputNode()
	{
		ObjectName = FText::FromString("Present");
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

	void ShaderToyOuputNode::Exec(GraphExecContext& Context)
	{

	}
}