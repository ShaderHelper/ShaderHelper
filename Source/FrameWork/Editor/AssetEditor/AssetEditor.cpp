#include "CommonHeader.h"
#include "AssetEditor.h"
#include "ProjectManager/ProjectManager.h"

namespace FW
{
	GLOBAL_REFLECTION_REGISTER(AddClass<AssetOp>())

	void AssetOp::OpenAsset(AssetObject* InAsset)
	{
		AssetOp* Op = GetAssetOp(InAsset->DynamicMetaType());
		Op->OnOpen(InAsset->GetPath());
	}

    AssetOp* GetAssetOp(const FString& InAssetPath)
    {
        return GetDefaultObject<AssetOp>([&](AssetOp* CurAssetOp){
            AssetObject* RelatedAssetDefaultObject = static_cast<AssetObject*>(CurAssetOp->SupportAsset()->GetDefaultObject());
			checkf(RelatedAssetDefaultObject, TEXT("Please ensure %s is constructible."), *CurAssetOp->SupportAsset()->TypeName);
            return RelatedAssetDefaultObject->FileExtension() ==  FPaths::GetExtension(InAssetPath);
        });
    }

	AssetOp* GetAssetOp(MetaType* InAssetMetaType)
	{
		return GetDefaultObject<AssetOp>([&](AssetOp* CurAssetOp) {
			return CurAssetOp->SupportAsset() == InAssetMetaType;
		});
	}

	void AssetOp::OnDelete(const FString& InAssetPath)
	{
		TSingleton<AssetManager>::Get().ClearAsset(InAssetPath);
		GProject->RemovePendingAsset(InAssetPath);
	}

}
