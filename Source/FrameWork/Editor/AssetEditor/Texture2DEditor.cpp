#include "CommonHeader.h"
#include "Texture2DEditor.h"
#include "AssetObject/Texture2D.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<Texture2DOp>()
						.BaseClass<AssetOp>();
	)

    ShReflectToy::MetaType* Texture2DOp::SupportAsset()
    {
        return ShReflectToy::GetMetaType<Texture2D>();
    }

	void Texture2DOp::Open(const FString& InAssetPath)
	{

	}
}
