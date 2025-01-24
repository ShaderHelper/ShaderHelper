#include "CommonHeader.h"
#include "ShaderToy.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToy>("ShaderToy Graph")
								.BaseClass<Graph>()
	)

    ShaderToy::~ShaderToy()
    {

    }

	void ShaderToy::Serialize(FArchive& Ar)
	{
		Graph::Serialize(Ar);
	}

	FString ShaderToy::FileExtension() const
	{
		return "shaderToy";
	}

	TArray<MetaType*> ShaderToy::SupportNodes() const
	{
		return {
			GetMetaType<ShaderToyOuputNode>(),
			GetMetaType<ShaderToyPassNode>()
		};
	}

}
