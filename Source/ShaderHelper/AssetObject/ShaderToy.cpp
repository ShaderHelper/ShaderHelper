#include "CommonHeader.h"
#include "ShaderToy.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderToy>()
								.BaseClass<Graph>()
	)

	ShaderToy::ShaderToy()
	{

	}

	void ShaderToy::Serialize(FArchive& Ar)
	{
		Graph::Serialize(Ar);
	}

	FString ShaderToy::FileExtension() const
	{
		return "ShaderToy";
	}

}