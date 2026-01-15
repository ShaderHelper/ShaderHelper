#include "CommonHeader.h"
#include "AssetEditor.h"
#include "ProjectManager/ProjectManager.h"

namespace FW
{
    REFLECTION_REGISTER(AddClass<AssetOp>()
								.BaseClass<ShObjectOp>()
	)

	void AssetOp::OpenAsset(AssetObject* InAsset)
	{
		AssetOp* Op = GetAssetOp(InAsset->DynamicMetaType());
		if(Op)
		{
			Op->OnOpen(InAsset->GetPath());
		}
	}

	void AssetOp::NavigateAsset(AssetObject* InAsset)
	{
		AssetOp* Op = GetAssetOp(InAsset->DynamicMetaType());
		if (Op)
		{
			Op->OnNavigate(InAsset->GetPath());
		}
	}

    AssetOp* GetAssetOp(const FString& InAssetPath)
    {
        return GetDefaultObject<AssetOp>([&](AssetOp* CurAssetOp){
            AssetObject* RelatedAssetDefaultObject = static_cast<AssetObject*>(CurAssetOp->SupportType()->GetDefaultObject());
			checkf(RelatedAssetDefaultObject, TEXT("Please ensure %s is constructible."), *CurAssetOp->SupportType()->TypeName);
            return RelatedAssetDefaultObject->FileExtension() ==  FPaths::GetExtension(InAssetPath);
        });
    }

	AssetOp* GetAssetOp(MetaType* InAssetMetaType)
	{
		return GetDefaultObject<AssetOp>([&](AssetOp* CurAssetOp) {
			return CurAssetOp->SupportType() == InAssetMetaType;
		});
	}

    void AssetOp::OnRename(const FString& OldPath, const FString& NewPath)
    {
        AssetObject* Asset = TSingleton<AssetManager>::Get().FindLoadedAsset(NewPath);
        if(Asset) {
            Asset->ObjectName = FText::FromString(FPaths::GetBaseFilename(NewPath));
        }
    }

	void AssetOp::OnDelete(const FString& InAssetPath)
	{
		TSingleton<AssetManager>::Get().DestroyAsset(InAssetPath);
	}

}
