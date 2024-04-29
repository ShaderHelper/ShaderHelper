#include "CommonHeader.h"
#include "Texture2DEditor.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<Texture2DOp>()
						.BaseClass<AssetOp>();
	)

	void Texture2DOp::Open(AssetObject* InObject)
	{

	}
}
