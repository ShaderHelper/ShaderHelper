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
		GraphNode::Serialize(Ar);
	}

	TArray<GraphPin*> ShaderToyPassNode::GetPins()
	{
		return { &PassOutput,&Slot1,&Slot2,&Slot3,&Slot4 };
	}

	void ShaderToyPassNode::Exec()
	{

	}

}