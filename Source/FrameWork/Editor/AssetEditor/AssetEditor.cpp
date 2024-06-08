#include "CommonHeader.h"
#include "AssetEditor.h"

namespace FRAMEWORK
{
	GLOBAL_REFLECTION_REGISTER(AddClass<AssetOp>())

    AssetOp* GetAssetOp(const FString& InAssetPath)
    {
        return GetDefaultObject<AssetOp>([&](AssetOp* CurAssetOp){
            AssetObject* RelatedAssetDefaultObject = static_cast<AssetObject*>(CurAssetOp->SupportAsset()->GetDefaultObject());
            return RelatedAssetDefaultObject->FileExtension() ==  FPaths::GetExtension(InAssetPath);
        });
    }
}
