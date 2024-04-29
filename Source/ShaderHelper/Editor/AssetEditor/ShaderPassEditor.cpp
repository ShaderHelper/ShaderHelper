#include "CommonHeader.h"
#include "ShaderPassEditor.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<ShaderPassOp>()
						.BaseClass<AssetOp>();
	)

	void ShaderPassOp::Open(AssetObject* InObject)
	{

	}
}
