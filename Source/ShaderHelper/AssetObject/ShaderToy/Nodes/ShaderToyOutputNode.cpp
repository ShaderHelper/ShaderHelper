#include "CommonHeader.h"
#include "ShaderToyOutputNode.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToyOuputNode>("ShaderToyOuputNode")
		.BaseClass<GraphNode>()
	)

	void ShaderToyOuputNode::Serialize(FArchive& Ar)
	{

	}

	TArray<GraphPin*> ShaderToyOuputNode::GetPins()
	{
		return { &ResultPin };
	}

	void ShaderToyOuputNode::Exec()
	{

	}
}