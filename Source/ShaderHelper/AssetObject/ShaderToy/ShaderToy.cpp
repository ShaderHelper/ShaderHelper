#include "CommonHeader.h"
#include "ShaderToy.h"
#include "Nodes/ShaderToyOutputNode.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToy>()
								.BaseClass<Graph>()
	)

	void ShaderToy::Serialize(FArchive& Ar)
	{
		Graph::Serialize(Ar);
	}

	FString ShaderToy::FileExtension() const
	{
		return "shaderToy";
	}


	void ShaderToy::PostLoad()
	{
		if (NodeDatas.IsEmpty())
		{
			AddNode(MakeShared<ShaderToyOuputNode>());
		}
	}

}