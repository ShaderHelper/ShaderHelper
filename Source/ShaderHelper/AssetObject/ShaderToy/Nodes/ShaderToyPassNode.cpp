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
		
		Slot0.Serialize(Ar);
		PassOutput.Serialize(Ar);
	}

	TArray<GraphPin*> ShaderToyPassNode::GetPins()
	{
		return { &PassOutput,&Slot0};
	}

	void ShaderToyPassNode::Exec()
	{

	}

}