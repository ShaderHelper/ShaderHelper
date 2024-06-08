#include "CommonHeader.h"
#include "Texture2DEditor.h"
#include "AssetObject/Texture2D.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(AddClass<Texture2DOp>()
                                .BaseClass<AssetOp>()
	)

    MetaType* Texture2DOp::SupportAsset()
    {
        return GetMetaType<Texture2D>();
    }

	void Texture2DOp::OnOpen(const FString& InAssetPath)
	{

	}
}
