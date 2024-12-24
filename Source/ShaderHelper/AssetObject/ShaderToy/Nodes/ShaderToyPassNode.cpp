#include "CommonHeader.h"
#include "ShaderToyPassNode.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToyPassNode>("ShaderToyPassNode")
		.BaseClass<GraphNode>()
	)

	void ShaderToyPassNode::Serialize(FArchive& Ar)
	{

	}

	TArray<GraphPin*> ShaderToyPassNode::GetPins()
	{

	}

	void ShaderToyPassNode::Exec()
	{

	}

}