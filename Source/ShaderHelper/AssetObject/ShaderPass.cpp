#include "CommonHeader.h"
#include "ShaderPass.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<ShaderPass>()
						.BaseClass<AssetObject>();
	)

	ShaderPass::ShaderPass()
	{

	}

	void ShaderPass::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << ShaderText;
	}

	FString ShaderPass::FileExtension() const
	{
		return "ShaderPass";
	}

	FSlateBrush* ShaderPass::GetImage() const
	{
		return nullptr;
	}

}